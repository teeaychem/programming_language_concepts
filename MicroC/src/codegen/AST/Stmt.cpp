#include <vector>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "AST/AST.hpp"
#include "AST/Node/Stmt.hpp"
#include "codegen/LLVMBundle.hpp"

// Stmt

// Blocks are generated in 'canonical' form:
//
// - Fresh declarations: declarations whose name does not occurr in any enclosing scope.
// - Shadowed declarations: declarations whose name *does* occurr in any enclosing scope.
// - All other statements
//
// Generation of statements stops immediately when a `return` statement is found.
//
// Codegen mutates the env with any declarations and restore the env on exit of the block.
llvm::Value *AST::Stmt::Block::codegen(LLVMBundle &bundle) const {

  std::vector<std::pair<std::string, llvm::Value *>> shadowed_values{};

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(bundle);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    auto x = std::make_pair(shadow_dec->declaration->name(), bundle.env_llvm.vars[shadow_dec->declaration->name()]);
    shadowed_values.push_back(x);

    shadow_dec->codegen(bundle);
  }

  for (auto &stmt : block.statements) {
    stmt->codegen(bundle);

    if (stmt->returns()) {
      break;
    }
  }

  // Unshadow shadowed vars
  for (auto &shadowed_var : shadowed_values) {
    bundle.env_llvm.vars[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    bundle.env_llvm.vars.erase(fresh_var->declaration->name());
  }

  return bundle.stmt_return_val();
}

// Declaration codegen redirects to the declaration.
llvm::Value *AST::Stmt::Declaration::codegen(LLVMBundle &bundle) const {
  return this->declaration->codegen(bundle);
}

// Expression codegen redirects to the expression.
llvm::Value *AST::Stmt::Expr::codegen(LLVMBundle &bundle) const {
  return this->expr->codegen(bundle);
}

// If codegen builds directs control flow to the appropriate block.
// Here, the `then` block is always generated as part of `if` codegen.

// An `else` block is generated if it is present in the AST.
//
// And, control returns to the source block only if it's possible by the semantics of the program.
llvm::Value *AST::Stmt::If::codegen(LLVMBundle &bundle) const {

  llvm::Function *parent = bundle.builder.GetInsertBlock()->getParent();

  // Condition evaluation
  llvm::Value *condition = this->condition->codegen_eval_true(bundle);

  // Flow block setup
  llvm::BasicBlock *block_then = llvm::BasicBlock::Create(*bundle.context, "if.then", parent);
  llvm::BasicBlock *block_else{}; // Build only if required
  llvm::BasicBlock *block_end = llvm::BasicBlock::Create(*bundle.context, "if.end");

  if (this->stmt_else->block.empty()) {
    bundle.builder.CreateCondBr(condition, block_then, block_end);
  } else {
    block_else = llvm::BasicBlock::Create(*bundle.context, "if.else");
    bundle.builder.CreateCondBr(condition, block_then, block_else);
  }

  bundle.builder.SetInsertPoint(block_then);
  llvm::Value *true_eval = this->stmt_then->codegen(bundle);

  if (!this->stmt_then->returns()) { //
    bundle.builder.CreateBr(block_end);
  }

  if (block_else) {
    parent->insert(parent->end(), block_else);
    bundle.builder.SetInsertPoint(block_else);
    llvm::Value *false_eval = this->stmt_else->codegen(bundle);

    if (!this->stmt_else->returns()) {
      bundle.builder.CreateBr(block_end);
    }
  }

  if (block_else == nullptr || !this->stmt_then->returns() || !this->stmt_else->returns()) {
    parent->insert(parent->end(), block_end);
    bundle.builder.SetInsertPoint(block_end);
  }

  return bundle.stmt_return_val();
}

// Non void fns set return storage in the `return_alloca` variable.
// This variable is empty otherwise.
//
// Note, in particular, nested blocks in an fn will all access the same return_alloca.
llvm::Value *AST::Stmt::Return::codegen(LLVMBundle &bundle) const {

  // If the return has some value...
  if (this->value.has_value()) {
    // Ensure the return value is loaded
    auto [return_val, _] = bundle.access(this->value.value().get());

    // Use a return allocatio if available, and break to the corresponding block
    if (bundle.env_llvm.return_alloca) {
      bundle.builder.CreateStore(return_val, bundle.env_llvm.return_alloca);
      bundle.builder.CreateBr(bundle.env_llvm.return_block);
    }
    // Otherwise, create a return
    else {
      bundle.builder.CreateRet(return_val);
    }
  }
  // Otherwise, the return must be void
  else {
    bundle.builder.CreateRetVoid();
  }

  return bundle.stmt_return_val();
}

// While codegen mirrors, mostly, if codegen
llvm::Value *AST::Stmt::While::codegen(LLVMBundle &bundle) const {
  llvm::Function *parent = bundle.builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *block_cond = llvm::BasicBlock::Create(*bundle.context, "while.cond", parent);
  llvm::BasicBlock *block_loop = llvm::BasicBlock::Create(*bundle.context, "while.loop");
  llvm::BasicBlock *block_end = llvm::BasicBlock::Create(*bundle.context, "while.end");

  bundle.builder.CreateBr(block_cond);
  bundle.builder.SetInsertPoint(block_cond);

  llvm::Value *condition = this->condition->codegen_eval_true(bundle);
  bundle.builder.CreateCondBr(condition, block_loop, block_end);

  parent->insert(parent->end(), block_loop);
  bundle.builder.SetInsertPoint(block_loop);

  this->body->codegen(bundle);
  if (!this->body->returns()) { // If entered, the while body may return...
    bundle.builder.CreateBr(block_cond);
  }

  parent->insert(parent->end(), block_end);
  bundle.builder.SetInsertPoint(block_end);

  return bundle.stmt_return_val();
}
