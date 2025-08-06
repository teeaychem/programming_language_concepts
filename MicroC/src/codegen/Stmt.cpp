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

// Stmt

Value *AST::Stmt::Block::codegen(LLVMBundle &hdl) const {

  std::vector<std::pair<std::string, Value *>> shadowed_values{};

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(hdl);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    auto x = std::make_pair(shadow_dec->declaration->name(), hdl.env_llvm.vars[shadow_dec->declaration->name()]);
    shadowed_values.push_back(x);

    shadow_dec->codegen(hdl);
  }

  for (auto &stmt : block.statements) {
    stmt->codegen(hdl);

    if (stmt->returns()) {
      break;
    }
  }

  // Unshadow shadowed vars
  for (auto &shadowed_var : shadowed_values) {
    hdl.env_llvm.vars[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    hdl.env_llvm.vars.erase(fresh_var->declaration->name());
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::Declaration::codegen(LLVMBundle &hdl) const {
  return this->declaration->codegen(hdl);
}

Value *AST::Stmt::Expr::codegen(LLVMBundle &hdl) const {
  return this->expr->codegen(hdl);
}

Value *AST::Stmt::If::codegen(LLVMBundle &hdl) const {

  Function *parent = hdl.builder.GetInsertBlock()->getParent();

  // Condition evaluation
  Value *condition = this->condition->codegen_eval_true(hdl);

  // Flow block setup
  BasicBlock *block_then = BasicBlock::Create(*hdl.context, "if.then", parent);
  BasicBlock *block_else{}; // Build only if required
  BasicBlock *block_end = BasicBlock::Create(*hdl.context, "if.end");

  if (this->stmt_else->block.empty()) {
    hdl.builder.CreateCondBr(condition, block_then, block_end);
  } else {
    block_else = BasicBlock::Create(*hdl.context, "if.else");
    hdl.builder.CreateCondBr(condition, block_then, block_else);
  }

  hdl.builder.SetInsertPoint(block_then);
  Value *true_eval = this->stmt_then->codegen(hdl);

  if (!this->stmt_then->returns()) { //
    hdl.builder.CreateBr(block_end);
  }
  // true_block = hdl.builder.GetInsertBlock(); // update for PHI

  if (block_else) {
    parent->insert(parent->end(), block_else);
    hdl.builder.SetInsertPoint(block_else);
    Value *false_eval = this->stmt_else->codegen(hdl);

    if (!this->stmt_else->returns()) {
      hdl.builder.CreateBr(block_end);
    }
    // false_block = hdl.builder.GetInsertBlock();  // update for PHI
  }

  if (block_else == nullptr || !this->stmt_then->returns() || !this->stmt_else->returns()) {
    parent->insert(parent->end(), block_end);
    hdl.builder.SetInsertPoint(block_end);
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::Return::codegen(LLVMBundle &hdl) const {

  if (this->value.has_value()) {

    auto [return_val, return_typ] = hdl.access(this->value.value().get());

    if (hdl.return_alloca) {
      hdl.builder.CreateStore(return_val, hdl.return_alloca);
      hdl.builder.CreateBr(hdl.return_block);
    } else {
      hdl.builder.CreateRet(return_val);
    }

  } else {
    hdl.builder.CreateRetVoid();
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::While::codegen(LLVMBundle &hdl) const {
  Function *parent = hdl.builder.GetInsertBlock()->getParent();

  BasicBlock *block_cond = BasicBlock::Create(*hdl.context, "while.cond", parent);
  BasicBlock *block_loop = BasicBlock::Create(*hdl.context, "while.loop");
  BasicBlock *block_end = BasicBlock::Create(*hdl.context, "while.end");

  hdl.builder.CreateBr(block_cond);
  hdl.builder.SetInsertPoint(block_cond);

  Value *condition = this->condition->codegen_eval_true(hdl);
  hdl.builder.CreateCondBr(condition, block_loop, block_end);

  parent->insert(parent->end(), block_loop);
  hdl.builder.SetInsertPoint(block_loop);
  this->body->codegen(hdl);
  if (!this->body->returns()) { // If entered, the while body may return...
    hdl.builder.CreateBr(block_cond);
  }

  parent->insert(parent->end(), block_end);
  hdl.builder.SetInsertPoint(block_end);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}
