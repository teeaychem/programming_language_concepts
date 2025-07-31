#pragma once

#include <map>
#include <string>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"

typedef llvm::Function *OpFoundation;
typedef std::map<const std::string, OpFoundation> OpsFoundationMap;

typedef std::function<llvm::Value *(AST::ExprHandle, AST::ExprHandle)> FnBinary;
typedef std::map<AST::Expr::OpBinary, FnBinary> OpsBinaryMap;

struct LLVMBundle;

void extend_ops_foundation(LLVMBundle &bundle, OpsFoundationMap &op_map);
void extend_ops_binary(LLVMBundle &bundle, OpsBinaryMap &op_map);

struct LLVMEnv {
  std::map<std::string, llvm::Value *> vars{};
  std::map<std::string, llvm::Function *> fns{};
};

struct LLVMBundle {

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  LLVMEnv env{};

  llvm::BasicBlock *return_block{nullptr};
  llvm::Value *return_alloca{nullptr};

  // Maps to fn builders
  OpsFoundationMap foundation_fn_map{};
  OpsBinaryMap prim2_fn_map{};

  // Utils
  llvm::Value *ensure_loaded(AST::TypHandle typ, llvm::Value *value);

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)) {

    extend_ops_foundation(*this, this->foundation_fn_map);
    extend_ops_binary(*this, this->prim2_fn_map);
  };
};

typedef std::unique_ptr<LLVMBundle> LLVMBundleHandle;
