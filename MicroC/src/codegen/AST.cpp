
#include "AST/AST.hpp"
#include "LLVMBundle.hpp"

#include "AST/Node/Access.hpp"
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

// Support

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *create_fn_alloca(Function *fn, StringRef name, Type *typ) {
  IRBuilder<> TmpB(&fn->getEntryBlock(), fn->getEntryBlock().begin());
  return TmpB.CreateAlloca(typ, nullptr, name);
}

// Access

Value *AST::Access::Var::codegen(LLVMBundle &hdl) const {
  auto it = hdl.named_values.find(this->var);
  if (it == hdl.named_values.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

// Expr

Value *AST::Expr::Access::codegen(LLVMBundle &hdl) const {
  std::cout << "Access codegen "
            << "\ttype: " << this->type()->to_string(0) << "\n"
            << "\tstring: " << this->to_string(0) << "\n"
            << "\n";

  Value *return_value;

  auto value = this->acc->codegen(hdl);
  auto typ = this->acc->eval_type()->typegen(hdl);

  return_value = value;

  return return_value;
}

Value *AST::Expr::Assign::codegen(LLVMBundle &hdl) const {
  Value *value = this->expr->codegen(hdl);
  Value *destination = this->dest->codegen(hdl);
  hdl.builder.CreateStore(value, destination);

  return value;
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
    std::cerr << "Unexpected unary op: " << this->op << "\n";
    std::exit(-1);
  }
}

Value *AST::Expr::Prim2::codegen(LLVMBundle &hdl) const {

  if (hdl.prim2_fn_map[this->op]) {
    return hdl.prim2_fn_map[this->op](a, b);
  } else {
    std::cerr << "Unexpected binary op: " << this->op << "\n";
    std::exit(-1);
  }
}

// Stmt

Value *AST::Stmt::Block::codegen(LLVMBundle &hdl) const {

  std::vector<std::pair<std::string, Value *>> shadowed_values{};

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(hdl);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    auto x = std::make_pair(shadow_dec->declaration->name(), hdl.named_values[shadow_dec->declaration->name()]);
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
    hdl.named_values[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    hdl.named_values.erase(fresh_var->declaration->name());
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

    Value *return_value = this->value.value()->codegen(hdl);

    if (hdl.return_alloca) {
      hdl.builder.CreateStore(return_value, hdl.return_alloca);
      hdl.builder.CreateBr(hdl.return_block);
    } else {
      hdl.builder.CreateRet(return_value);
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
  this->stmt->codegen(hdl);
  if (!this->stmt->returns()) { // If entered, the while body may return...
    hdl.builder.CreateBr(block_cond);
  }

  parent->insert(parent->end(), block_end);
  hdl.builder.SetInsertPoint(block_end);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

// Dec

// Code generation for a declaration.
// Should always be called when a declaration is made.
// The details of shadowing are handled at block nodes.
Value *AST::Dec::Var::codegen(LLVMBundle &hdl) const {
  auto existing = hdl.named_values.find(this->name());
  if (existing != hdl.named_values.end()) {
    return existing->second;
  }

  auto typ = this->typ->typegen(hdl);

  if (this->typ->kind() == Typ::Kind::Array) {
    if (this->scope != Scope::Local) {
      throw std::logic_error("Global declaration of an array");
    }

    auto typ_array = std::static_pointer_cast<Typ::TypIndex>(this->typ);

    ArrayType *array_type = ArrayType::get(typ_array->expr_type()->typegen(hdl), typ_array->size.value_or(0));

    auto size_value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), typ_array->size.value_or(0));

    auto alloca = hdl.builder.CreateAlloca(typ, size_value, this->name());
    hdl.named_values[this->name()] = alloca;
    return alloca;
  }

  // Non-array variables ...

  auto value = this->typ->defaultgen(hdl);

  switch (this->scope) {

  case Scope::Local: {
    auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
    hdl.builder.CreateStore(value, alloca);

    hdl.named_values[this->name()] = alloca;
  } break;

  case Scope::Global: {
    auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);

    GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
    globalVar->setInitializer(value);

    hdl.named_values[this->name()] = globalVar;
  } break;
  }

  return value; // Return the default value for type, as it's initialised
}

Value *AST::Dec::Fn::codegen(LLVMBundle &hdl) const {
  if (hdl.named_values.count(this->name()) != 0) {
    throw std::logic_error(std::format("Redeclaration of function: {}", this->name()));
  }

  BasicBlock *outer_return_block = hdl.return_block; // stash any existing return block, to be restored on exit
  Value *outer_return_alloca = hdl.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  llvm::Type *return_type = this->r_typ->typegen(hdl);
  std::vector<llvm::Type *> parameter_types{};

  { // Generate the parameter types
    parameter_types.reserve(this->params.size());

    for (auto &p : this->params) {
      parameter_types.push_back(p.first->typegen(hdl));
    }
  }

  auto fn_type = FunctionType::get(return_type, parameter_types, false);
  Function *fn = Function::Create(fn_type, Function::ExternalLinkage, this->id, hdl.module.get());

  BasicBlock *fn_body = BasicBlock::Create(*hdl.context, "entry", fn);
  hdl.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->params[name_idx++].second;

      arg.setName(base_name);

      AllocaInst *alloca = create_fn_alloca(fn, base_name, arg.getType());
      hdl.builder.CreateStore(&arg, alloca);

      auto it = hdl.named_values.find(base_name);
      if (it != hdl.named_values.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, alloca));
      } else {
        fresh_parameters.push_back(base_name);
      }

      hdl.named_values[base_name] = alloca;
    }
  }

  // Return setup
  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      AllocaInst *r_alloca = create_fn_alloca(fn, "ret.val", return_type);
      hdl.return_alloca = r_alloca;
    }

    auto return_block = BasicBlock::Create(*hdl.context, "return");
    hdl.return_block = return_block;
  }

  hdl.builder.SetInsertPoint(fn_body);

  this->body->codegen(hdl);

  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      fn->insert(fn->end(), hdl.return_block);
      hdl.builder.SetInsertPoint(hdl.return_block);

      auto *return_value = hdl.builder.CreateLoad(return_type, hdl.return_alloca);
      hdl.builder.CreateRet(return_value);
    }
  } else if (return_type->isVoidTy()) {
    hdl.builder.CreateRetVoid();
  }

  hdl.return_block = outer_return_block;
  hdl.return_alloca = outer_return_alloca;

  for (auto &shadowed : shadowed_parameters) {
    hdl.named_values[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    hdl.named_values.erase(fresh);
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

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
