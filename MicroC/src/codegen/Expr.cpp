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
#include "AST/Node/Expr.hpp"
#include "AST/Types.hpp"

#include "LLVMBundle.hpp"

using namespace llvm;

// Nodes

Value *AST::Expr::Var::codegen(LLVMBundle &bundle) const {
  auto it = bundle.env.vars.find(this->var);
  if (it == bundle.env.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

Value *AST::Expr::Call::codegen(LLVMBundle &bundle) const {
  Function *callee_f = bundle.module->getFunction(this->name);

  std::vector<Value *> arg_values{};

  if (callee_f == nullptr) {
    throw std::logic_error(std::format("Call to unknown function: {}", this->name));
  } else if (callee_f->arg_size() != this->arguments.size()) {
    throw std::logic_error(std::format("Argument size mismatch."));
  }

  for (auto &arg : this->arguments) {
    auto arg_value = arg->codegen(bundle);
    if (arg_value == nullptr) {
      throw std::logic_error(std::format("Failed to process argument: {}", arg->to_string(0)));
    }

    arg_value = bundle.access_if(arg, arg_value);

    arg_values.push_back(arg_value);
  }

  return bundle.builder.CreateCall(callee_f, arg_values);
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
    // return bundle.builder.CreateLoad(expr->type()->llvm(bundle), );
    return expr->codegen(bundle);
  } break;

  case OpUnary::Sub: {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.access_if(expr, expr_val);

    return bundle.builder.CreateNeg(expr_val, "op.sub");
  } break;

  case OpUnary::Negation: {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.access_if(expr, expr_val);

    return bundle.builder.CreateNot(expr_val, "op.neg");
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
    return bundle.builder.CreateCmp(ICmpInst::ICMP_NE, evaluation, zero);
  }
}

Value *AST::ExprT::codegen_eval_false(LLVMBundle &bundle) const {
  Value *evaluation = this->codegen(bundle);

  if (evaluation->getType()->isIntegerTy(1)) {
    return evaluation;
  } else {
    Value *zero = ConstantInt::get(this->type()->llvm(bundle), 0);
    return bundle.builder.CreateCmp(ICmpInst::ICMP_EQ, evaluation, zero);
  }
}

// Binary ops

namespace OpBinaryCodegen {

llvm::Value *builder_assign(LLVMBundle &bundle, AST::ExprHandle destination, AST::ExprHandle value) {

  llvm::Value *destination_val = destination->codegen(bundle);

  llvm::Value *value_val = value->codegen(bundle);

  value_val = bundle.access_if(value, value_val);

  bundle.builder.CreateStore(value_val, destination_val, "op.assign");

  return value_val;
}

// Codegen for ptr + int
llvm::Value *builder_ptr_add(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto ptr_typ = ptr->type()->deref()->llvm(bundle);
  auto ptr_elem = bundle.access_if(val, val->codegen(bundle));

  auto ptr_sub = bundle.builder.CreateGEP(ptr_typ,
                                          ptr->codegen(bundle),
                                          ArrayRef<Value *>(ptr_elem));

  return ptr_sub;
}

llvm::Value *builder_add(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  auto lhs_type = expr->lhs->type();
  auto rhs_type = expr->rhs->type();

  switch (expr->type()->kind()) {

  case AST::Typ::Kind::Ptr: {

    if (expr->lhs->evals_to(AST::Typ::Kind::Ptr) && expr->rhs->evals_to(AST::Typ::Kind::Int)) {
      return builder_ptr_add(bundle, expr->lhs, expr->rhs);
    }

    else if (expr->lhs->evals_to(AST::Typ::Kind::Int) && expr->rhs->evals_to(AST::Typ::Kind::Ptr)) {
      return builder_ptr_add(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  case AST::Typ::Kind::Int: {

    llvm::Value *lhs_val = expr->lhs->codegen(bundle);
    llvm::Value *rhs_val = expr->rhs->codegen(bundle);

    lhs_val = bundle.access_if(expr->lhs, lhs_val);
    rhs_val = bundle.access_if(expr->rhs, rhs_val);

    return bundle.builder.CreateAdd(lhs_val, rhs_val, "op.add");

  } break;

  case AST::Typ::Kind::Char: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;
  }
}

// Codegen for ptr - int
llvm::Value *builder_ptr_sub(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto val_value = bundle.access_if(val, val->codegen(bundle));

  auto ptr_typ = ptr->type()->deref()->llvm(bundle);
  auto ptr_elem = bundle.builder.CreateNeg(val_value);

  auto ptr_sub = bundle.builder.CreateGEP(ptr_typ,
                                          ptr->codegen(bundle),
                                          ArrayRef<Value *>(ptr_elem));

  return ptr_sub;
}

llvm::Value *builder_sub(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  auto lhs_type = expr->lhs->type();
  auto rhs_type = expr->rhs->type();

  switch (expr->type()->kind()) {

  case AST::Typ::Kind::Ptr: {

    if (expr->lhs->evals_to(AST::Typ::Kind::Ptr) && expr->rhs->evals_to(AST::Typ::Kind::Int)) {
      return builder_ptr_sub(bundle, expr->lhs, expr->rhs);
    }

    else if (expr->lhs->evals_to(AST::Typ::Kind::Int) && expr->rhs->evals_to(AST::Typ::Kind::Ptr)) {
      return builder_ptr_sub(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  case AST::Typ::Kind::Int: {

    llvm::Value *lhs_val = expr->lhs->codegen(bundle);
    llvm::Value *rhs_val = expr->rhs->codegen(bundle);

    lhs_val = bundle.access_if(expr->lhs, lhs_val);
    rhs_val = bundle.access_if(expr->rhs, rhs_val);

    return bundle.builder.CreateSub(lhs_val, rhs_val, "op.sub");

  } break;

  case AST::Typ::Kind::Char: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;
  }
}

llvm::Value *builder_mul(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateMul(lhs_val, rhs_val, "op.mul");
}

llvm::Value *builder_div(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateSDiv(lhs_val, rhs_val, "op.div");
}

llvm::Value *builder_mod(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateSRem(lhs_val, rhs_val, "op.mod");
}

llvm::Value *builder_eq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, lhs_val, rhs_val);
}

llvm::Value *builder_ne(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, lhs_val, rhs_val);
}

llvm::Value *builder_gt(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, lhs_val, rhs_val);
}

llvm::Value *builder_lt(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, lhs_val, rhs_val);
}

llvm::Value *builder_geq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, lhs_val, rhs_val);
}

llvm::Value *builder_leq(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, lhs_val, rhs_val);
}

llvm::Value *builder_and(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateAnd(lhs_val, rhs_val);
}

llvm::Value *builder_or(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  llvm::Value *lhs_val = expr->lhs->codegen(bundle);
  llvm::Value *rhs_val = expr->rhs->codegen(bundle);

  lhs_val = bundle.access_if(expr->lhs, lhs_val);
  rhs_val = bundle.access_if(expr->rhs, rhs_val);

  return bundle.builder.CreateOr(lhs_val, rhs_val);
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
