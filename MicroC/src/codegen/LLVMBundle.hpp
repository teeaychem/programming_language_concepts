#pragma once

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <map>
#include <memory>

typedef llvm::Function *OpFoundation;
typedef std::map<const std::string, OpFoundation> OpsFoundationMap;

typedef std::function<llvm::Value *(llvm::Value *)> OpUnary;
typedef std::map<const std::string, OpUnary> OpsUnaryMap;

typedef std::function<llvm::Value *(llvm::Value *, llvm::Value *)> OpBinary;
typedef std::map<const std::string, OpBinary> OpsBinaryMap;

struct LLVMBundle;

void extend_ops_foundation(LLVMBundle &bundle, OpsFoundationMap &op_map);
void extend_ops_unary(LLVMBundle &bundle, OpsUnaryMap &op_map);
void extend_ops_binary(LLVMBundle &bundle, OpsBinaryMap &op_map);

struct LLVMBundle {

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  std::map<std::string, llvm::Value *> named_values{};

  llvm::BasicBlock *return_block{nullptr};
  llvm::Value *return_alloca{nullptr};

  // Maps to fn builders
  OpsFoundationMap foundation_fn_map{};
  OpsUnaryMap prim1_fn_map{};
  OpsBinaryMap prim2_fn_map{};

  void setup_ops_binary();

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)),

        named_values({}) {

    extend_ops_foundation(*this, this->foundation_fn_map);
    extend_ops_unary(*this, this->prim1_fn_map);
    extend_ops_binary(*this, this->prim2_fn_map);
  };
};

typedef std::unique_ptr<LLVMBundle> LLVMBundleHandle;
