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

// #include "AST/Fmt.hpp"

using namespace llvm;

// Support

// Access

Value *AST::Expr::Var::codegen(LLVMBundle &hdl) const {
  auto it = hdl.env.vars.find(this->var);
  if (it == hdl.env.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

// Expr

// Value *AST::Expr::Assign::codegen(LLVMBundle &hdl) const {
//   Value *value = this->expr->codegen(hdl);
//   Value *destination = this->dest->codegen(hdl);
//   hdl.builder.CreateStore(value, destination);

//   return value;
// }

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
    auto x = std::make_pair(shadow_dec->declaration->name(), hdl.env.vars[shadow_dec->declaration->name()]);
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
    hdl.env.vars[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    hdl.env.vars.erase(fresh_var->declaration->name());
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
    return_value = hdl.ensure_loaded(this->value.value()->type(), return_value);

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
  this->body->codegen(hdl);
  if (!this->body->returns()) { // If entered, the while body may return...
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
  auto existing = hdl.env.vars.find(this->name());
  if (existing != hdl.env.vars.end()) {
    return existing->second;
  }

  auto typ = this->typ->typegen(hdl);

  switch (this->typ->kind()) {

  case Typ::Kind::Array: {

    auto as_array = std::static_pointer_cast<Typ::TypIndex>(this->typ);

    ArrayType *array_type = ArrayType::get(as_array->expr_type()->typegen(hdl), as_array->size.value_or(0));

    switch (this->scope) {

    case Scope::Local: {

      auto size_value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), as_array->size.value_or(0));
      auto alloca = hdl.builder.CreateAlloca(typ, size_value, this->name());
      hdl.env.vars[this->name()] = alloca;

      return alloca;
    } break;

    case Scope::Global: {

      auto alloca = hdl.module->getOrInsertGlobal(this->name(), array_type);
      GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
      ConstantAggregateZero *array_init = ConstantAggregateZero::get(array_type);
      globalVar->setInitializer(array_init);
      hdl.env.vars[this->name()] = globalVar;

      return globalVar;

    } break;
    }
  } break;

  case Typ::Kind::Data: {

    auto as_data = std::static_pointer_cast<Typ::TypData>(this->typ);
    auto value = as_data->defaultgen(hdl);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
      hdl.builder.CreateStore(value, alloca);
      hdl.env.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
      globalVar->setInitializer(value);
      hdl.env.vars[this->name()] = globalVar;

    } break;
    }

    return value;

  } break;

  case Typ::Kind::Pointer: {

    auto as_ptr = std::static_pointer_cast<Typ::TypPointer>(this->typ);
    auto value = as_ptr->defaultgen(hdl);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
      hdl.builder.CreateStore(value, alloca);
      hdl.env.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
      globalVar->setInitializer(value);
      hdl.env.vars[this->name()] = globalVar;

    } break;
    }

    return value;

  } break;
  }
}

// tmp

Value *AST::Dec::Prototype::codegen(LLVMBundle &hdl) const {
  if (hdl.env.vars.count(this->name()) != 0) {
    throw std::logic_error(std::format("Redeclaration of function: {}", this->name()));
  }

  llvm::Type *return_type = this->return_type()->typegen(hdl);
  std::vector<llvm::Type *> parameter_types{};

  { // Generate the parameter types
    parameter_types.reserve(this->params.size());

    for (auto &p : this->params) {
      parameter_types.push_back(p.second->typegen(hdl));
    }
  }

  auto fn_type = FunctionType::get(return_type, parameter_types, false);
  Function *fnx = Function::Create(fn_type, Function::ExternalLinkage, this->id, hdl.module.get());

  hdl.env.fns[this->id] = fnx;

  return fnx;
}

//

Value *AST::Dec::Fn::codegen(LLVMBundle &hdl) const {

  Function *fn = (Function *)this->prototype->codegen(hdl);

  // Fn details

  llvm::Type *return_type = this->return_type()->typegen(hdl);

  BasicBlock *outer_return_block = hdl.return_block; // stash any existing return block, to be restored on exit
  Value *outer_return_alloca = hdl.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  BasicBlock *fn_body = BasicBlock::Create(*hdl.context, "entry", fn);
  hdl.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->prototype->params[name_idx++].first;

      arg.setName(base_name);

      AllocaInst *alloca = hdl.builder.CreateAlloca(arg.getType(), nullptr, base_name);

      hdl.builder.CreateStore(&arg, alloca);

      auto it = hdl.env.vars.find(base_name);
      if (it != hdl.env.vars.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, alloca));
      } else {
        fresh_parameters.push_back(base_name);
      }

      hdl.env.vars[base_name] = alloca;
    }
  }

  // Return setup
  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      AllocaInst *r_alloca = hdl.builder.CreateAlloca(return_type, nullptr, "ret.val");
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
    hdl.env.vars[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    hdl.env.vars.erase(fresh);
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
