#pragma once

#include <map>
#include <string>

#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"

// A primative function, linked to a module (somehow)
struct FnPrimative {
  // The name of the function, used in both source and IR
  std::string name;

  // The return type of the function
  AST::TypHandle return_type;

  // Arguments to the fn.
  AST::VarTypVec args;

  // LLVM IR codegen for the function.
  virtual llvm::Function *codegen(Context &ctx) const = 0;

  // For the LLVM `addGlobalMapping` method.
  // Currently unused.
  virtual int64_t global_map_addr() const = 0;
};

// Ccontextually relevant information for codegen.
// As with the EnvAST struct, this is mutated to maintain information about variables in scope, etc.
// And, in particular, it is up to codegen methods to appropriately maintain the struct.
// See codegen for blocks or fn declarations for examples of significant maintenance.
struct EnvLLVM {
  // variables in scope
  std::map<std::string, llvm::Value *> vars{};

  // fns in scope (as fns are global, this is each)
  std::map<std::string, llvm::Function *> fns{};

  // block designated for return codegen for fns, to pass control to
  llvm::BasicBlock *return_block{nullptr};

  // alloca to write a return value to
  llvm::Value *return_alloca{nullptr};
};

// Objects and general methods for codegen.
struct Context {

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

  // Contextully 'access' `expr`, returning the codegen'd value and corresponding AST type.
  // Variables, pointers, etc., are not loaded in order to support assignment.
  // The `access` method typically performs a load if required.
  // Called on the rhs of an assignment, or when passing a value to an fn.
  std::pair<llvm::Value *, AST::TypHandle> access(AST::ExprT const *expr);

  Context()
      : context(std::make_unique<llvm::LLVMContext>()),
        module(std::make_unique<llvm::Module>("microC", *context)),
        builder(llvm::IRBuilder<>(*context)) {

    this->populate_foundation_fn_map();

    for (auto &foundation_elem : this->foundation_fn_map) {
      auto primative_fn = foundation_elem.second;

      AST::Dec::Prototype proto(primative_fn->return_type, primative_fn->name, primative_fn->args);
      AST::Dec::PrototypeHandle handle = std::make_shared<AST::Dec::Prototype>(proto);

      this->env_ast.fns[primative_fn->name] = handle;
    }
  };

  // The type which would be returned with a codegen'd value by calling `access` on `expr`.
  AST::TypHandle access_type(AST::ExprT const *expr);

  // Populates the foundation fn map, to be called on initialisation of this.
  // Defined together with the fns.
  void populate_foundation_fn_map();

  // Canonical codegen types
  // Used with `codegen` on types, with the exception of pointers which capture area information.
  llvm::Type *get_typ(AST::Typ::Kind kind) {

    llvm::Type *return_typ;

    switch (kind) {

    case AST::Typ::Kind::Bool: {
      // Booleans are Int1 to match LLVM returns to comparisons, etc.
      return_typ = llvm::Type::getInt1Ty(*this->context);
    } break;

    case AST::Typ::Kind::Char: {
      // As standard.
      return_typ = llvm::Type::getInt8Ty(*this->context);
    } break;

    case AST::Typ::Kind::Int: {
      // An arbitrary choice over Int32.
      return_typ = llvm::Type::getInt64Ty(*this->context);
    } break;

    case AST::Typ::Kind::Ptr: {
      // Generally unqualified, following current LLVM trends.
      // Still, calling `codegen` on an AST type is preferred to obtain information about arrays, etc.
      return_typ = llvm::PointerType::getUnqual(*this->context);
    } break;

    case AST::Typ::Kind::Void: {
      return_typ = llvm::Type::getVoidTy(*this->context);
    } break;
    }

    return return_typ;
  }

  // Returns an zero of type int.
  llvm::Value *get_zero() { return llvm::ConstantInt::get(this->get_typ(AST::Typ::Kind::Int), 0); }

  // The return value for a statement.
  // As inaccessible, a null value of void type.
  llvm::Value *stmt_return_val() {
    auto void_type = llvm::Type::getVoidTy(*this->context);
    return llvm::Constant::getNullValue(void_type);
  }
};
