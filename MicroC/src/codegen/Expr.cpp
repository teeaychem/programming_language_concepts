#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Fmt.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"

#include "LLVMBundle.hpp"

using namespace llvm;

// Nodes

Value *AST::Expr::Var::codegen(LLVMBundle &bundle) const {
  auto it = bundle.env_llvm.vars.find(this->var);
  if (it == bundle.env_llvm.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

Value *AST::Expr::Call::codegen(LLVMBundle &bundle) const {

  Function *callee_f = bundle.module->getFunction(this->name);

  if (callee_f == nullptr) {
    auto it = bundle.foundation_fn_map.find(this->name);
    if (it != bundle.foundation_fn_map.end()) {
      callee_f = it->second->codegen(bundle);
    } else {
      throw std::logic_error(std::format("Call to unknown fn: {}", this->name));
    }
  } else if (callee_f->arg_size() != this->arguments.size()) {
    throw std::logic_error(std::format("Call to {} requires {} arguments, received {}.",
                                       this->name, callee_f->arg_size(), this->arguments.size()));
  }

  std::cout << "Finding... " << this->name << "\n";
  auto fn_ast = bundle.env_ast.fns.find(this->name);
  if (fn_ast != bundle.env_ast.fns.end()) {
    std::cout << "in ast: " << fn_ast->second->to_string();
  }
  std::cout << "\n";

  std::vector<Value *> arg_values{};
  for (auto &arg : this->arguments) {
    arg_values.push_back(bundle.access(arg));
  }

  return bundle.builder.CreateCall(callee_f, arg_values, std::format("{}", this->name));
}

Value *AST::Expr::Cast::codegen(LLVMBundle &bundle) const {

  switch (this->type()->kind()) {

  case Typ::Kind::Bool: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;

  case Typ::Kind::Char: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;

  case Typ::Kind::Int: {

    switch (expr->type()->kind()) {

    case Typ::Kind::Bool: {
      auto expr_value = this->expr->codegen(bundle);
      auto cast = bundle.builder.CreateIntCast(expr_value, this->type()->llvm(bundle), false);
      return cast;
    } break;

    case Typ::Kind::Char:
    case Typ::Kind::Int: {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    } break;

    case Typ::Kind::Ptr: {
      auto ptr_value = this->expr->codegen(bundle);
      auto cast = bundle.builder.CreatePtrToInt(ptr_value, this->type()->llvm(bundle));
      return cast;
    } break;

    case Typ::Kind::Void: {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    } break;
    }

  } break;

  case Typ::Kind::Ptr: {
    if (this->expr->is_of_type(AST::Typ::Kind::Int)) {
      auto int_value = this->expr->codegen(bundle);
      auto cast = bundle.builder.CreateIntToPtr(int_value, this->type()->llvm(bundle));
      return cast;
    } else {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    }
  } break;

  case Typ::Kind::Void: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;
  }
}

Value *AST::Expr::CstI::codegen(LLVMBundle &bundle) const {
  return ConstantInt::get(this->type()->llvm(bundle), this->i, true);
}

Value *AST::Expr::Index::codegen(LLVMBundle &bundle) const {
  Value *value = this->access->codegen(bundle);

  // TODO: Improve
  assert(value->getType()->isPointerTy());

  Type *typ = this->access->type()->llvm(bundle);
  Value *index = this->index->codegen(bundle);

  auto ptr = bundle.builder.CreateGEP(typ, value, ArrayRef<Value *>(index));

  return ptr;
}

Value *AST::Expr::Prim1::codegen(LLVMBundle &bundle) const {

  switch (this->op) {

  case OpUnary::AddressOf: {
    return expr->codegen(bundle);
  } break;

  case OpUnary::Dereference: {
    return expr->codegen(bundle);
  } break;

  case OpUnary::Sub: {
    return bundle.builder.CreateNeg(bundle.access(expr), "op.neg");
  } break;

  case OpUnary::Negation: {
    return bundle.builder.CreateNot(bundle.access(expr), "op.not");
  } break;
  }
}

// Support

Value *AST::ExprT::codegen_eval_true(LLVMBundle &bundle) const {
  Value *evaluation = this->codegen(bundle);

  if (evaluation->getType()->isIntegerTy(1)) {
    return evaluation;
  } else {
    Value *zero = ConstantInt::get(this->type()->llvm(bundle), 0);
    return bundle.builder.CreateCmp(ICmpInst::ICMP_NE, evaluation, zero, "op.neq");
  }
}

Value *AST::ExprT::codegen_eval_false(LLVMBundle &bundle) const {
  Value *evaluation = this->codegen(bundle);

  if (evaluation->getType()->isIntegerTy(1)) {
    return evaluation;
  } else {
    Value *zero = ConstantInt::get(this->type()->llvm(bundle), 0);
    return bundle.builder.CreateCmp(ICmpInst::ICMP_EQ, evaluation, zero, "op.eq");
  }
}

// Binary ops

namespace OpBinaryCodegen {

void throw_unsupported(const AST::Expr::Prim2 *expr) {
  throw std::logic_error(std::format("Cannot perform '{}' on types '{}' and '{}'",
                                     expr->op,
                                     expr->lhs->type()->to_string(),
                                     expr->rhs->type()->to_string()));
}

llvm::Value *builder_assign(LLVMBundle &bundle, AST::ExprHandle destination, AST::ExprHandle value) {

  llvm::Value *destination_val = destination->codegen(bundle);

  auto value_val = bundle.access(value);

  bundle.builder.CreateStore(value_val, destination_val, "op.assign");

  return value_val;
}

// Codegen for ptr + int
llvm::Value *builder_ptr_add(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto ptr_typ = ptr->type()->deref()->llvm(bundle);

  auto ptr_sub = bundle.builder.CreateGEP(ptr_typ,
                                          ptr->codegen(bundle),
                                          ArrayRef<Value *>(bundle.access(val)));

  return ptr_sub;
}

llvm::Value *builder_add(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  switch (expr->type()->kind()) {

  case AST::Typ::Kind::Bool: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Char: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto lhs_val = bundle.access(expr->lhs);
    auto rhs_val = bundle.access(expr->rhs);

    return bundle.builder.CreateAdd(lhs_val, rhs_val, "op.add");

  } break;

  case AST::Typ::Kind::Ptr: {

    if (expr->lhs->is_of_type(AST::Typ::Kind::Ptr) && expr->rhs->is_of_type(AST::Typ::Kind::Int)) {
      return builder_ptr_add(bundle, expr->lhs, expr->rhs);
    }

    else if (expr->lhs->is_of_type(AST::Typ::Kind::Int) && expr->rhs->is_of_type(AST::Typ::Kind::Ptr)) {
      return builder_ptr_add(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;
  }
}

// Codegen for ptr - int
llvm::Value *builder_ptr_sub(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto ptr_typ = ptr->type()->deref()->llvm(bundle);
  auto ptr_elem = bundle.builder.CreateNeg(bundle.access(val));

  auto ptr_sub = bundle.builder.CreateGEP(ptr_typ,
                                          ptr->codegen(bundle),
                                          ArrayRef<Value *>(ptr_elem));

  return ptr_sub;
}

llvm::Value *builder_sub(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  switch (expr->type()->kind()) {

  case AST::Typ::Kind::Bool: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Char: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto lhs_val = bundle.access(expr->lhs);
    auto rhs_val = bundle.access(expr->rhs);

    return bundle.builder.CreateSub(lhs_val, rhs_val, "op.sub");

  } break;

  case AST::Typ::Kind::Ptr: {

    if (expr->lhs->is_of_type(AST::Typ::Kind::Ptr) && expr->rhs->is_of_type(AST::Typ::Kind::Int)) {
      return builder_ptr_sub(bundle, expr->lhs, expr->rhs);
    }

    else if (expr->lhs->is_of_type(AST::Typ::Kind::Int) && expr->rhs->is_of_type(AST::Typ::Kind::Ptr)) {
      return builder_ptr_sub(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;
  }
}

llvm::Value *builder_mul(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  if (!expr->lhs->is_of_type(AST::Typ::Kind::Int) || !expr->rhs->is_of_type(AST::Typ::Kind::Int)) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateMul(lhs_val, rhs_val, "op.mul");
}

llvm::Value *builder_div(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(AST::Typ::Kind::Int) || !expr->rhs->is_of_type(AST::Typ::Kind::Int)) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateSDiv(lhs_val, rhs_val, "op.div");
}

llvm::Value *builder_mod(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(AST::Typ::Kind::Int) || !expr->rhs->is_of_type(AST::Typ::Kind::Int)) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateSRem(lhs_val, rhs_val, "op.mod");
}

llvm::Value *builder_eq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, lhs_val, rhs_val, "op.eq");
}

llvm::Value *builder_ne(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, lhs_val, rhs_val, "op.neq");
}

llvm::Value *builder_gt(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, lhs_val, rhs_val, "op.gt");
}

llvm::Value *builder_lt(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, lhs_val, rhs_val, "op.lt");
}

llvm::Value *builder_geq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, lhs_val, rhs_val, "op.geq");
}

llvm::Value *builder_leq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  if (!expr->lhs->is_of_type(expr->rhs->type()->kind())) {
    throw_unsupported(expr);
  }

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, lhs_val, rhs_val, "op.leq");
}

llvm::Value *builder_and(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateAnd(lhs_val, rhs_val, "op.and");
}

llvm::Value *builder_or(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  auto lhs_val = bundle.access(expr->lhs);
  auto rhs_val = bundle.access(expr->rhs);

  return bundle.builder.CreateOr(lhs_val, rhs_val, "op.or");
}
} // namespace OpBinaryCodegen

Value *AST::Expr::Prim2::codegen(LLVMBundle &bundle) const {

  switch (op) {

  case OpBinary::Assign: {
    return OpBinaryCodegen::builder_assign(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::AssignAdd: {
    throw std::logic_error("todo: +=");
  } break;
  case OpBinary::AssignSub: {
    throw std::logic_error("todo: -=");
  } break;
  case OpBinary::AssignMul: {
    throw std::logic_error("todo: *=");
  } break;
  case OpBinary::AssignDiv: {
    throw std::logic_error("todo: /=");
  } break;
  case OpBinary::AssignMod: {
    throw std::logic_error("todo: %=");
  } break;
  case OpBinary::Add: {
    return OpBinaryCodegen::builder_add(bundle, this);
  } break;
  case OpBinary::Sub: {
    return OpBinaryCodegen::builder_sub(bundle, this);
  } break;
  case OpBinary::Mul: {
    return OpBinaryCodegen::builder_mul(bundle, this);
  } break;
  case OpBinary::Div: {
    return OpBinaryCodegen::builder_div(bundle, this);
  } break;
  case OpBinary::Mod: {
    return OpBinaryCodegen::builder_mod(bundle, this);
  } break;
  case OpBinary::Eq: {
    return OpBinaryCodegen::builder_eq(bundle, this);
  } break;
  case OpBinary::Neq: {
    return OpBinaryCodegen::builder_ne(bundle, this);
  } break;
  case OpBinary::Gt: {
    return OpBinaryCodegen::builder_gt(bundle, this);
  } break;
  case OpBinary::Lt: {
    return OpBinaryCodegen::builder_lt(bundle, this);
  } break;
  case OpBinary::Leq: {
    return OpBinaryCodegen::builder_geq(bundle, this);
  } break;
  case OpBinary::Geq: {
    return OpBinaryCodegen::builder_leq(bundle, this);
  } break;
  case OpBinary::And: {
    return OpBinaryCodegen::builder_and(bundle, this);
  } break;
  case OpBinary::Or: {
    return OpBinaryCodegen::builder_or(bundle, this);
  } break;
  }
}
