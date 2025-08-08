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

  // Arguments to the fn.
  AST::ArgVec args;

  // LLVM IR codegen for the function.
  virtual llvm::Function *codegen(LLVMBundle &bundle) const = 0;

  // For the LLVM `addGlobalMapping` method.
  // Currently unused.
  virtual int64_t global_map_addr() const = 0;
};

// A struct containing contextually relevant information for codegen.
// As with the EnvAST struct, this is mutated to maintain information about variables in scope, etc.
// And, in particular, it is up to codegen methods to appropriately maintain the struct.
// See codegen for blocks or fn declarations for examples of significant maintenance.
struct EnvLLVM {
  // variables in scope
  std::map<std::string, llvm::Value *> vars{};
  // fns in scope (as fns are global, this is each)
  std::map<std::string, llvm::Function *> fns{};
  // contextual return block
  llvm::BasicBlock *return_block{nullptr};
  // contextual alloca to write a return value to
  llvm::Value *return_alloca{nullptr};
};

struct LLVMBundle {

  std::unique_ptr<llvm::LLVMContext> context;
  std::unique_ptr<llvm::Module> module;
  llvm::IRBuilder<> builder;

  // See above.
  EnvLLVM env_llvm{};

  // Fn / Prototype / Variable to type mapping maintained during parsing.
  // Empty before, keep after parsing.
  AST::EnvAST env_ast{};

  // Maps to fn builders
  std::map<const std::string, std::shared_ptr<FnPrimative>> foundation_fn_map{};

  // Utils

  // Contextully 'access' `expr`, returning the codegen'd value and corresponding AST type.
  // Variables, pointers, etc., are not loaded in order to support assignment.
  // The `access` method typically performs a load if required.
  // Called on the rhs of an assignment, or when passing a value to an fn.
  std::pair<llvm::Value *, AST::TypHandle> access(AST::ExprT const *expr);

  // The type which would be returned with a codegen'd value by calling `access` on `expr`.
  AST::TypHandle access_type(AST::ExprT const *expr);

  void generate_foundation_fn_map();

  LLVMBundle()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)) {

    this->generate_foundation_fn_map();
  };

  // Canonical codegen types
  // Used with `codegen` on types, with the exception of pointers which capture area information.
  llvm::Type *get_typ(AST::Typ::Kind kind) {
    switch (kind) {

    case AST::Typ::Kind::Bool: {
      // Booleans are Int1 to match LLVM returns to comparisons, etc.
      return llvm::Type::getInt1Ty(*this->context);
    } break;

    case AST::Typ::Kind::Char: {
      // As standard.
      return llvm::Type::getInt8Ty(*this->context);
    } break;

    case AST::Typ::Kind::Int: {
      // An arbitrary choice over Int32.
      return llvm::Type::getInt64Ty(*this->context);
    } break;

    case AST::Typ::Kind::Ptr: {
      // Generally unqualified, following current LLVM trends.
      // Still, calling `codegen` on an AST type is preferred to obtain information about arrays, etc.
      return llvm::PointerType::getUnqual(*this->context);
    } break;

    case AST::Typ::Kind::Void: {
      return llvm::Type::getVoidTy(*this->context);
    } break;
    }
  }

  // Returns an zero of type int.
  llvm::Value *get_zero() {
    return llvm::ConstantInt::get(this->get_typ(AST::Typ::Kind::Int), 0);
  }

  // The return value for a statement.
  // Unused in practice.
  llvm::Value *stmt_return_val() { return this->get_zero(); }
};
