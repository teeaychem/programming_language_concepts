#include <vector>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "AST/AST.hpp"
#include "AST/Node/Stmt.hpp"
#include "codegen/Structs.hpp"

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
llvm::Value *AST::Stmt::Block::codegen(Context &ctx) const {

  std::vector<std::pair<std::string, llvm::Value *>> shadowed_values{};

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(ctx);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    auto var = shadow_dec->declaration->var();
    auto x = std::make_pair(var, ctx.env_llvm.vars[var]);
    shadowed_values.push_back(x);
    auto y = shadow_dec->codegen(ctx);
  }

  for (auto &stmt : block.statements) {
    stmt->codegen(ctx);

    if (stmt->returns()) {
      break;
    }
  }

  // Unshadow shadowed vars
  for (auto &shadowed_var : shadowed_values) {
    ctx.env_llvm.vars[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    ctx.env_llvm.vars.erase(fresh_var->declaration->var());
  }

  return ctx.stmt_return_val();
}

// Declaration codegen redirects to the declaration.
llvm::Value *AST::Stmt::Declaration::codegen(Context &ctx) const {
  return this->declaration->codegen(ctx);
}

// Expression codegen redirects to the expression.
llvm::Value *AST::Stmt::Expr::codegen(Context &ctx) const {
  return this->expr->codegen(ctx);
}

// If codegen builds directs control flow to the appropriate block.
// Here, the `then` block is always generated as part of `if` codegen.

// An `else` block is generated if it is present in the AST.
//
// And, control returns to the source block only if it's possible by the semantics of the program.
llvm::Value *AST::Stmt::If::codegen(Context &ctx) const {

  llvm::Function *parent = ctx.builder.GetInsertBlock()->getParent();

  // Condition evaluation
  llvm::Value *condition = this->condition->codegen_eval_true(ctx);

  // Flow block setup
  llvm::BasicBlock *block_then = llvm::BasicBlock::Create(*ctx.context, "if.then", parent);
  llvm::BasicBlock *block_else{}; // Build only if required
  llvm::BasicBlock *block_end = llvm::BasicBlock::Create(*ctx.context, "if.end");

  if (this->stmt_else->block.empty()) {
    ctx.builder.CreateCondBr(condition, block_then, block_end);
  } else {
    block_else = llvm::BasicBlock::Create(*ctx.context, "if.else");
    ctx.builder.CreateCondBr(condition, block_then, block_else);
  }

  ctx.builder.SetInsertPoint(block_then);
  llvm::Value *true_eval = this->stmt_then->codegen(ctx);

  if (!this->stmt_then->returns()) { //
    ctx.builder.CreateBr(block_end);
  }

  if (block_else) {
    parent->insert(parent->end(), block_else);
    ctx.builder.SetInsertPoint(block_else);
    llvm::Value *false_eval = this->stmt_else->codegen(ctx);

    if (!this->stmt_else->returns()) {
      ctx.builder.CreateBr(block_end);
    }
  }

  if (block_else == nullptr || !this->stmt_then->returns() || !this->stmt_else->returns()) {
    parent->insert(parent->end(), block_end);
    ctx.builder.SetInsertPoint(block_end);
  }

  return ctx.stmt_return_val();
}

// Non void fns set return storage in the `return_alloca` variable.
// This variable is empty otherwise.
//
// Note, in particular, nested blocks in an fn will all access the same return_alloca.
llvm::Value *AST::Stmt::Return::codegen(Context &ctx) const {

  // If the return has some value...
  if (this->value.has_value()) {
    // Ensure the return value is loaded
    auto [return_val, _] = ctx.access(this->value.value().get());

    // Use a return allocatio if available, and break to the corresponding block
    if (ctx.env_llvm.return_alloca) {
      ctx.builder.CreateStore(return_val, ctx.env_llvm.return_alloca);
      ctx.builder.CreateBr(ctx.env_llvm.return_block);
    }
    // Otherwise, create a return
    else {
      ctx.builder.CreateRet(return_val);
    }
  }
  // Otherwise, the return must be void
  else {
    ctx.builder.CreateRetVoid();
  }

  return ctx.stmt_return_val();
}

// While codegen mirrors, mostly, if codegen
llvm::Value *AST::Stmt::While::codegen(Context &ctx) const {
  llvm::Function *parent = ctx.builder.GetInsertBlock()->getParent();

  llvm::BasicBlock *block_cond = llvm::BasicBlock::Create(*ctx.context, "while.cond", parent);
  llvm::BasicBlock *block_loop = llvm::BasicBlock::Create(*ctx.context, "while.loop");
  llvm::BasicBlock *block_end = llvm::BasicBlock::Create(*ctx.context, "while.end");

  ctx.builder.CreateBr(block_cond);
  ctx.builder.SetInsertPoint(block_cond);

  llvm::Value *condition = this->condition->codegen_eval_true(ctx);
  ctx.builder.CreateCondBr(condition, block_loop, block_end);

  parent->insert(parent->end(), block_loop);
  ctx.builder.SetInsertPoint(block_loop);

  this->body->codegen(ctx);
  if (!this->body->returns()) { // If entered, the while body may return...
    ctx.builder.CreateBr(block_cond);
  }

  parent->insert(parent->end(), block_end);
  ctx.builder.SetInsertPoint(block_end);

  return ctx.stmt_return_val();
}
