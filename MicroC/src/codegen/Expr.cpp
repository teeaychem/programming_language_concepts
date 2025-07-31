#include "AST/AST.hpp"
#include "LLVMBundle.hpp"

#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "AST/Fmt.hpp"

using namespace llvm;

// Expr

Value *AST::Expr::Var::codegen(LLVMBundle &hdl) const {
  auto it = hdl.env.vars.find(this->var);
  if (it == hdl.env.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

Value *AST::Expr::Call::codegen(LLVMBundle &hdl) const {
  Function *callee_f = hdl.module->getFunction(this->name);

  std::vector<Value *> arg_values{};

  if (callee_f == nullptr) {
    throw std::logic_error(std::format("Call to unknown function: {}", this->name));
  } else if (callee_f->arg_size() != this->arguments.size()) {
    throw std::logic_error(std::format("Argument size mismatch."));
  }

  for (auto &arg : this->arguments) {
    auto arg_value = arg->codegen(hdl);
    if (arg_value == nullptr) {
      throw std::logic_error(std::format("Failed to process argument: {}", arg->to_string(0)));
    }

    arg_value = hdl.ensure_loaded(arg->type(), arg_value);

    arg_values.push_back(arg_value);
  }

  return hdl.builder.CreateCall(callee_f, arg_values);
}

Value *AST::Expr::CstI::codegen(LLVMBundle &hdl) const {
  return ConstantInt::get(this->type()->typegen(hdl), this->i, true);
}

Value *AST::Expr::Index::codegen(LLVMBundle &hdl) const {
  Value *value = this->access->codegen(hdl);

  // TODO: Improve
  assert(value->getType()->isPointerTy());

  Type *typ = this->access->type()->typegen(hdl);

  Value *index = this->index->codegen(hdl);

  auto ptr = hdl.builder.CreateGEP(typ, value, ArrayRef<Value *>(index));

  return ptr;
}

Value *AST::Expr::Prim1::codegen(LLVMBundle &hdl) const {

  if (hdl.prim1_fn_map[this->op]) {
    return hdl.prim1_fn_map[this->op](expr);
  } else {
    throw std::logic_error(std::format("Unexpected unary op: {}", this->op));
  }
}

Value *AST::Expr::Prim2::codegen(LLVMBundle &hdl) const {

  if (hdl.prim2_fn_map[this->op]) {
    return hdl.prim2_fn_map[this->op](lhs, rhs);
  } else {
    throw std::logic_error(std::format("Unexpected binary op: {}", this->op));
  }
}

//

Value *AST::ExprT::codegen_eval_true(LLVMBundle &hdl) const {
  Value *evaluation = this->codegen(hdl);

  if (evaluation->getType()->isIntegerTy(1)) {
    return evaluation;
  } else {
    Value *zero = ConstantInt::get(this->type()->typegen(hdl), 0);
    return hdl.builder.CreateCmp(ICmpInst::ICMP_NE, evaluation, zero);
  }
}

Value *AST::ExprT::codegen_eval_false(LLVMBundle &hdl) const {
  Value *evaluation = this->codegen(hdl);

  if (evaluation->getType()->isIntegerTy(1)) {
    return evaluation;
  } else {
    Value *zero = ConstantInt::get(this->type()->typegen(hdl), 0);
    return hdl.builder.CreateCmp(ICmpInst::ICMP_EQ, evaluation, zero);
  }
}
