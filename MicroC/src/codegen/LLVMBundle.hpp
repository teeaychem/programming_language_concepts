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
  AST::ArgVec args;

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
  std::pair<llvm::Value *, AST::TypHandle> access(AST::ExprT const *expr);

  AST::TypHandle access_type(AST::ExprT const *expr);

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)) {

    extend_ops_foundation(*this);
  };

  // Canonical codegen types
  // Used with `codegen` on types, with the exception of pointers which capture area information.
  llvm::Type *get_typ(AST::Typ::Kind kind) {
    switch (kind) {

    case AST::Typ::Kind::Bool: {
      return llvm::Type::getInt1Ty(*this->context);
    } break;

    case AST::Typ::Kind::Char: {
      return llvm::Type::getInt8Ty(*this->context);
    } break;

    case AST::Typ::Kind::Int: {
      return llvm::Type::getInt64Ty(*this->context);
    } break;

    case AST::Typ::Kind::Ptr: {
      return llvm::PointerType::getUnqual(*this->context);
    } break;

    case AST::Typ::Kind::Void: {
      return llvm::Type::getVoidTy(*this->context);
    } break;
    }
  }

  llvm::Value *get_zero() {
    return llvm::ConstantInt::get(this->get_typ(AST::Typ::Kind::Int), 0);
  }

  llvm::Value *stmt_return_val() { return this->get_zero(); }
};
