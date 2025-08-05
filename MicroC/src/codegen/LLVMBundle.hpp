#pragma once

#include <map>
#include <string>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"

// A primative function, linked to a module (somehow)
struct FnPrimative {
  // The name of the function, used in both source and IR
  std::string name;

  // The return type of the function
  AST::TypHandle return_type;

  // Arguments
  AST::ParamVec args;

  // LLVM IR codegen for the function.
  virtual llvm::Function *codegen(LLVMBundle &bundle) const = 0;

  virtual int64_t global_map_addr() const = 0;
};

typedef std::map<const std::string, std::shared_ptr<FnPrimative>> OpPrimativeMap;

void extend_ops_foundation(LLVMBundle &bundle);

struct EnvLLVM {
  std::map<std::string, llvm::Value *> vars{};
  std::map<std::string, llvm::Function *> fns{};
};

struct LLVMBundle {

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  EnvLLVM env_llvm{};

  // Fn / Prototype / Variable to type mapping maintained during parsing.
  // Empty before, keep after parsing.
  AST::EnvAST env_ast{};

  llvm::BasicBlock *return_block{nullptr};
  llvm::Value *return_alloca{nullptr};

  // Maps to fn builders
  OpPrimativeMap foundation_fn_map{};

  // Utils
  llvm::Value *access(AST::ExprHandle expr, llvm::Value *value);
  llvm::Value *access(AST::ExprHandle expr);

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)) {

    extend_ops_foundation(*this);
  };
};

typedef std::unique_ptr<LLVMBundle> LLVMBundleHandle;
