#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Fmt.hpp"
#include "AST/Node/Expr.hpp"

#include "codegen/LLVMBundle.hpp"

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
  auto prototype = bundle.env_ast.fns.find(this->name)->second.get();

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

  std::vector<Value *> arg_values{};
  for (size_t i = 0; i < this->arguments.size(); ++i) {

    auto [access_val, access_typ] = bundle.access(this->arguments[i].get());
    arg_values.push_back(access_val);
  }

  // Named instructions cannot provide void values
  return bundle.builder.CreateCall(callee_f, arg_values);
}

Value *AST::Expr::Cast::codegen(LLVMBundle &bundle) const {

  switch (this->type_kind()) {

  case Typ::Kind::Bool:
  case Typ::Kind::Char:
  case Typ::Kind::Void: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;

  case Typ::Kind::Int: {

    switch (expr->type_kind()) {

    case Typ::Kind::Bool: {
      auto cast = bundle.builder.CreateIntCast(this->expr->codegen(bundle),
                                               this->type()->codegen(bundle), false);
      return cast;
    } break;

    case Typ::Kind::Char:
    case Typ::Kind::Int:
    case Typ::Kind::Void: {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    } break;

    case Typ::Kind::Ptr: {
      auto cast = bundle.builder.CreatePtrToInt(this->expr->codegen(bundle),
                                                this->type()->codegen(bundle));
      return cast;
    } break;
    }

  } break;

  case Typ::Kind::Ptr: {
    if (this->expr->typ_has_kind(AST::Typ::Kind::Int)) {
      auto cast = bundle.builder.CreateIntToPtr(this->expr->codegen(bundle),
                                                this->type()->codegen(bundle));
      return cast;
    } else {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    }
  } break;
  }
}

Value *AST::Expr::CstI::codegen(LLVMBundle &bundle) const {
  return ConstantInt::get(this->type()->codegen(bundle), this->i, true);
}

Value *AST::Expr::Index::codegen(LLVMBundle &bundle) const {

  auto [ptr_val, ptr_typ] = bundle.access(this->target.get());
  auto [access_val, access_typ] = bundle.access(this->index.get());

  auto ptr_add = bundle.builder.CreateInBoundsGEP(ptr_typ->deref()->codegen(bundle),
                                                  ptr_val,
                                                  ArrayRef<Value *>{access_val},
                                                  "idx");

  return ptr_add;
}

Value *AST::Expr::Prim1::codegen(LLVMBundle &bundle) const {

  llvm::Value *return_value;

  switch (this->op) {

  case OpUnary::AddressOf: {
    // The address of a dereference is obtained by avoiding the dereference.
    // This is an instance of the general approach to never take the address of an object.
    return_value = expr->codegen(bundle);

  } break;

  case OpUnary::Dereference: {
    // Dereference performs a load / access.
    auto [access_val, _] = bundle.access(this->expr.get());
    return_value = access_val;
  } break;

  case OpUnary::Sub: {
    auto [access_val, _] = bundle.access(expr.get());
    return_value = bundle.builder.CreateNeg(access_val, "op.neg");
  } break;

  case OpUnary::Negation: {
    auto [access_val, _] = bundle.access(expr.get());
    return_value = bundle.builder.CreateNot(access_val, "op.not");
  } break;
  }

  return return_value;
}

// Support

Value *AST::ExprT::codegen_eval_true(LLVMBundle &bundle) const {

  llvm::Value *return_value;

  auto [access_val, access_typ] = bundle.access(this);

  // If Already a boolean...
  if (access_val->getType()->isIntegerTy(1)) {
    return_value = access_val;
  }
  // Otherwise test not equal to null val of expr type
  else {
    return_value = bundle.builder.CreateCmp(ICmpInst::ICMP_NE,
                                            access_val,
                                            access_typ->defaultgen(bundle),
                                            "op.eval_true");
  }

  return return_value;
}

Value *AST::ExprT::codegen_eval_false(LLVMBundle &bundle) const {

  llvm::Value *return_value;

  auto [access_val, access_typ] = bundle.access(this);

  if (access_val->getType()->isIntegerTy(1)) {
    return_value = access_val;
  } else {
    return_value = bundle.builder.CreateCmp(ICmpInst::ICMP_EQ,
                                            access_val,
                                            access_typ->defaultgen(bundle),
                                            "op.eval_false");
  }

  return return_value;
}

// Binary ops

namespace OpBinaryCodegen {

llvm::Value *builder_assign(LLVMBundle &bundle, AST::ExprHandle destination, AST::ExprHandle value) {

  llvm::Value *destination_val = destination->codegen(bundle);

  auto [access_val, access_typ] = bundle.access(value.get());
  bundle.builder.CreateStore(access_val, destination_val, "op.assign");

  return access_val;
}

// Codegen for ptr + int
llvm::Value *builder_ptr_add(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto [access_val, access_typ] = bundle.access(val.get());
  auto [ptr_val, ptr_typ] = bundle.access(ptr.get());
  auto ptr_add = bundle.builder.CreateInBoundsGEP(ptr_typ->deref()->codegen(bundle),
                                                  ptr_val,
                                                  ArrayRef<Value *>{access_val},
                                                  "add.ptr");

  return ptr_add;
}

llvm::Value *builder_add(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  llvm::Value *return_value;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto [lhs_val, lhs_typ] = bundle.access(expr->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(expr->rhs.get());

    return_value = bundle.builder.CreateAdd(lhs_val, rhs_val, "op.add");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      return_value = builder_ptr_add(bundle, expr->lhs, expr->rhs);
    }

    else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      return_value = builder_ptr_add(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  } break;
  }

  return return_value;
}

// Codegen for ptr - int
llvm::Value *builder_ptr_sub(LLVMBundle &bundle, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto [access_val, access_typ] = bundle.access(val.get());

  auto ptr_typ = ptr->type()->codegen(bundle);
  auto ptr_elem = bundle.builder.CreateNeg(access_val);
  auto ptr_sub = bundle.builder.CreateGEP(ptr_typ,
                                          ptr->codegen(bundle),
                                          ArrayRef<Value *>(ptr_elem),
                                          "sub");

  return ptr_sub;
}

llvm::Value *builder_sub(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {

  llvm::Value *return_value;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto [lhs_val, lhs_typ] = bundle.access(expr->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(expr->rhs.get());

    return_value = bundle.builder.CreateSub(lhs_val, rhs_val, "op.sub");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      return_value = builder_ptr_sub(bundle, expr->lhs, expr->rhs);
    }

    else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      return_value = builder_ptr_sub(bundle, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  break;
  }

  return return_value;
}

llvm::Value *builder_mul(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = bundle.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = bundle.access(expr->rhs.get());

  return bundle.builder.CreateMul(lhs_val, rhs_val, "op.mul");
}

llvm::Value *builder_div(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = bundle.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = bundle.access(expr->rhs.get());

  return bundle.builder.CreateSDiv(lhs_val, rhs_val, "op.div");
}

llvm::Value *builder_mod(LLVMBundle &bundle, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = bundle.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = bundle.access(expr->rhs.get());

  return bundle.builder.CreateSRem(lhs_val, rhs_val, "op.mod");
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
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, lhs_val, rhs_val, "op.eq");
  } break;

  case OpBinary::Neq: {
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, lhs_val, rhs_val, "op.neq");
  } break;

  case OpBinary::Gt: {
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, lhs_val, rhs_val, "op.gt");
  } break;

  case OpBinary::Lt: {
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, lhs_val, rhs_val, "op.lt");
  } break;

  case OpBinary::Leq: {
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, lhs_val, rhs_val, "op.leq");
  } break;

  case OpBinary::Geq: {
    auto [lhs_val, lhs_typ] = bundle.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = bundle.access(this->rhs.get());

    return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, lhs_val, rhs_val, "op.geq");
  } break;

  case OpBinary::And: {
    auto lhs_val = this->lhs->codegen_eval_true(bundle);
    auto rhs_val = this->rhs->codegen_eval_true(bundle);

    return bundle.builder.CreateAnd(lhs_val, rhs_val, "op.and");
  } break;

  case OpBinary::Or: {
    auto lhs_val = this->lhs->codegen_eval_true(bundle);
    auto rhs_val = this->rhs->codegen_eval_true(bundle);

    return bundle.builder.CreateOr(lhs_val, rhs_val, "op.or");
  } break;
  }
}
