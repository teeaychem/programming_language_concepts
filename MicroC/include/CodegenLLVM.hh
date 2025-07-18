#pragma once

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <map>
#include <memory>

struct LLVMBundle {

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  std::map<std::string, llvm::Value *> named_values{};

  llvm::BasicBlock *return_block{nullptr};
  llvm::Value* return_alloca{nullptr};

  std::map<std::string, llvm::Function *> base_fns{
      {"printf", llvm::Function::Create(llvm::FunctionType::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*this->context)), true), llvm::Function::ExternalLinkage, "printf", this->module.get())}};

  std::map<const std::string, std::function<llvm::Value *(llvm::Value *)>> prim1_fns{
      {"-", [this](llvm::Value *expr) { return this->builder.CreateMul(llvm::ConstantInt::get(expr->getType(), -1), expr, "sub"); }},
      {"printi", [this](llvm::Value *expr) {
         std::vector<llvm::Value *> arg_vs{
             this->builder.CreateGlobalString("%d\n", "digit_formatter"),
             expr,
         };

         return this->builder.CreateCall(this->base_fns["printf"], arg_vs);
       }},
      {"printc", [this](llvm::Value *expr) {
         std::vector<llvm::Value *> arg_vs{
             this->builder.CreateGlobalString("%c", "new_line"),
             expr};

         return this->builder.CreateCall(this->base_fns["printf"], arg_vs);
       }}

  };

  std::map<const std::string, std::function<llvm::Value *(llvm::Value *, llvm::Value *)>> prim2_fns{
      {"+", [this](llvm::Value *LHS, llvm::Value *RHS) { return this->builder.CreateAdd(LHS, RHS, "add"); }},
      {"-", [this](llvm::Value *LHS, llvm::Value *RHS) { return this->builder.CreateSub(LHS, RHS, "sub"); }},
      {"*", [this](llvm::Value *LHS, llvm::Value *RHS) { return this->builder.CreateMul(LHS, RHS, "mul"); }},
      {"/", [this](llvm::Value *LHS, llvm::Value *RHS) { return this->builder.CreateSub(LHS, RHS, "div"); }}};

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)),

        named_values({}) {};
};

typedef std::unique_ptr<LLVMBundle> LLVMBundleHandle;
