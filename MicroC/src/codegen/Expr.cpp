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

Value *AST::Expr::Prim2::codegen(LLVMBundle &bundle) const {

  if (bundle.prim2_fn_map[this->op]) {
    return bundle.prim2_fn_map[this->op](lhs, rhs);
  } else {
    throw std::logic_error(std::format("Unexpected binary op: {}", this->op));
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
