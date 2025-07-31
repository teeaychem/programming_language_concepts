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

    arg_value = bundle.ensure_loaded(arg->type(), arg_value);

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
    return bundle.builder.CreateLoad(expr->type()->llvm(bundle), expr->codegen(bundle));
  } break;

  case OpUnary::Sub: {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateMul(llvm::ConstantInt::get(expr_val->getType(), -1), expr_val, "sub");
  } break;

  case OpUnary::Negation: {
    llvm::Value *expr_val = expr->codegen(bundle);
    expr_val = bundle.ensure_loaded(expr->type(), expr_val);

    return bundle.builder.CreateNot(expr_val);
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
  value_val = bundle.ensure_loaded(value->type(), value_val);

  return bundle.builder.CreateStore(value_val, destination_val, "op.assign");
}

llvm::Value *builder_add(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {

  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateAdd(a_val, b_val, "op.add");
}

llvm::Value *builder_sub(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {

  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateSub(a_val, b_val, "op.sub");
}

llvm::Value *builder_mul(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {

  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateMul(a_val, b_val, "op.mul");
}

llvm::Value *builder_div(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateSDiv(a_val, b_val, "op.div");
}

llvm::Value *builder_mod(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateSRem(a_val, b_val, "op.mod");
}

llvm::Value *builder_eq(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, a_val, b_val);
}

llvm::Value *builder_ne(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, a_val, b_val);
}

llvm::Value *builder_gt(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, a_val, b_val);
}

llvm::Value *builder_lt(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, a_val, b_val);
}

llvm::Value *builder_geq(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, a_val, b_val);
}

llvm::Value *builder_leq(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, a_val, b_val);
}

llvm::Value *builder_and(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateAnd(a_val, b_val);
}

llvm::Value *builder_or(LLVMBundle &bundle, AST::ExprHandle a, AST::ExprHandle b) {
  llvm::Value *a_val = a->codegen(bundle);
  llvm::Value *b_val = b->codegen(bundle);

  a_val = bundle.ensure_loaded(a->type(), a_val);
  b_val = bundle.ensure_loaded(b->type(), b_val);

  return bundle.builder.CreateOr(a_val, b_val);
}
} // namespace OpBinaryCodegen

Value *AST::Expr::Prim2::codegen(LLVMBundle &bundle) const {

  switch (op) {

  case OpBinary::Assign: {
    return OpBinaryCodegen::builder_assign(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::AssignAdd: {
    throw std::logic_error("todo");
  } break;
  case OpBinary::AssignSub: {
    throw std::logic_error("todo");
  } break;
  case OpBinary::AssignMul: {
    throw std::logic_error("todo");
  } break;
  case OpBinary::AssignDiv: {
    throw std::logic_error("todo");
  } break;
  case OpBinary::AssignMod: {
    throw std::logic_error("todo");
  } break;
  case OpBinary::Add: {
    return OpBinaryCodegen::builder_add(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Sub: {
    return OpBinaryCodegen::builder_sub(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Mul: {
    return OpBinaryCodegen::builder_mul(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Div: {
    return OpBinaryCodegen::builder_div(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Mod: {
    return OpBinaryCodegen::builder_mod(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Eq: {
    return OpBinaryCodegen::builder_eq(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Neq: {
    return OpBinaryCodegen::builder_ne(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Gt: {
    return OpBinaryCodegen::builder_gt(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Lt: {
    return OpBinaryCodegen::builder_lt(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Leq: {
    return OpBinaryCodegen::builder_geq(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Geq: {
    return OpBinaryCodegen::builder_leq(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::And: {
    return OpBinaryCodegen::builder_and(bundle, this->lhs, this->rhs);
  } break;
  case OpBinary::Or: {
    return OpBinaryCodegen::builder_or(bundle, this->lhs, this->rhs);
  } break;
  }
}
