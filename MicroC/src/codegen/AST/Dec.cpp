#include <format>
#include <vector>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

// Dec

// Code generation for a declaration.
// Should always be called when a declaration is made.
// The details of shadowing are handled at block nodes.
llvm::Value *AST::Dec::Var::codegen(LLVMBundle &bundle) const {
  auto existing = bundle.env_llvm.vars.find(this->name());
  if (existing != bundle.env_llvm.vars.end()) {
    return existing->second;
  }

  auto typgen = this->typ->codegen(bundle);
  auto var = this->name();

  // codegen splits for each type, and in turn splits on local / global.
  // the type split is used to generate default values to assign to global and to use as return values.
  // defalt values are not stored for local declarations, in line with C.
  // (Though, mostly to reduce the amount of IR generated.)
  switch (this->typ->kind()) {

  case Typ::Kind::Ptr: {

    auto as_ptr = std::static_pointer_cast<Typ::Ptr>(this->typ);
    auto default_value = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*bundle.context));

    // Pointers split again on whether the pointer has some associated area.
    // That is, on a whether the pointer is (explicitly) to an array or not.
    if (as_ptr->area().has_value()) {

      llvm::ArrayType *array_typ = llvm::ArrayType::get(as_ptr->pointee_type()->codegen(bundle), as_ptr->area().value());

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = bundle.builder.CreateAlloca(typgen, nullptr, var); // Create
        bundle.env_llvm.vars[var] = alloca;                              // Update env

      } break;

      case Scope::Global: {

        bundle.module->getOrInsertGlobal(var, array_typ);                                // Create
        llvm::GlobalVariable *globalVar = bundle.module->getNamedGlobal(var);            // Find
        llvm::ConstantAggregateZero *init = llvm::ConstantAggregateZero::get(array_typ); // Init a.
        globalVar->setInitializer(init);                                                 // Init b.
        bundle.env_llvm.vars[var] = globalVar;                                           // Update env

      } break;
      }

      return default_value;
    }

    else {

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = bundle.builder.CreateAlloca(typgen, nullptr, var);
        bundle.env_llvm.vars[var] = alloca;

      } break;

      case Scope::Global: {

        bundle.module->getOrInsertGlobal(var, typgen);
        llvm::GlobalVariable *globalVar = bundle.module->getNamedGlobal(var);
        globalVar->setInitializer(default_value);
        bundle.env_llvm.vars[var] = globalVar;

      } break;
      }

      return default_value;
    }

  } break;

  case Typ::Kind::Int: {

    auto as_int = std::static_pointer_cast<AST::Typ::Int>(this->typ);
    auto default_value = as_int->defaultgen(bundle);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = bundle.builder.CreateAlloca(typgen, nullptr, var);
      bundle.env_llvm.vars[var] = alloca;

    } break;

    case Scope::Global: {

      bundle.module->getOrInsertGlobal(var, typgen);
      llvm::GlobalVariable *globalVar = bundle.module->getNamedGlobal(var);
      globalVar->setInitializer(default_value);
      bundle.env_llvm.vars[var] = globalVar;

    } break;
    }

    return default_value;

  } break;

  case Typ::Kind::Char: {

    auto as_char = std::static_pointer_cast<AST::Typ::Char>(this->typ);
    auto default_value = as_char->defaultgen(bundle);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = bundle.builder.CreateAlloca(typgen, nullptr, var);
      bundle.env_llvm.vars[var] = alloca;

    } break;

    case Scope::Global: {

      bundle.module->getOrInsertGlobal(var, typgen);
      llvm::GlobalVariable *globalVar = bundle.module->getNamedGlobal(var);
      globalVar->setInitializer(default_value);
      bundle.env_llvm.vars[var] = globalVar;

    } break;
    }

    return default_value;
  }

  case Typ::Kind::Void: {
    throw std::logic_error("Creation of variable with void type");
  } break;

  case Typ::Kind::Bool: {
    throw std::logic_error("Creation of variable with bool type");
  } break;
  }
}

// Prototype
// Return the fn as the implementation doesn't permit prototypes independent of a body declaration.
// If revised to do so, the fn generation should be abstracted as this is called during fn declaration.
llvm::Value *AST::Dec::Prototype::codegen(LLVMBundle &bundle) const {
  llvm::Type *return_type = this->return_type()->codegen(bundle);
  std::vector<llvm::Type *> parameter_types{};

  // Generate the parameter types
  parameter_types.reserve(this->args.size());
  for (auto &p : this->args) {
    parameter_types.push_back(p.typ->codegen(bundle));
  }

  auto fn_type = llvm::FunctionType::get(return_type, parameter_types, false);
  llvm::Function *fn = llvm::Function::Create(fn_type, llvm::Function::ExternalLinkage, this->id, bundle.module.get());

  bundle.env_llvm.fns[this->id] = fn;

  return fn;
}

// Fn
// The fn codegen is sourced to prototype codegen, though see comments for issues with this.
// The codegen given is for the body.
// This amounts to setting up local allocas for each parameter, generating the body, and maintaining the env.
//
// Returns are handled by setting `return_alloca` in the bundle to the local return alloca.
// This helps simplify control structure, as any return makes a store and then (something equivalent to) a jump to the end of the fn.
llvm::Value *AST::Dec::Fn::codegen(LLVMBundle &bundle) const {

  llvm::Function *fn = (llvm::Function *)this->prototype->codegen(bundle);

  // Fn details

  llvm::Type *return_type = this->return_type()->codegen(bundle);

  llvm::BasicBlock *outer_return_block = bundle.env_llvm.return_block; // to be restored on exit
  llvm::Value *outer_return_alloca = bundle.env_llvm.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, llvm::Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  llvm::BasicBlock *fn_body = llvm::BasicBlock::Create(*bundle.context, "entry", fn);
  bundle.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {
      auto &base_name = this->prototype->args[name_idx].var;
      auto &base_type = this->prototype->args[name_idx].typ;

      auto it = bundle.env_llvm.vars.find(base_name);
      if (it != bundle.env_llvm.vars.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, it->second));
      } else {
        fresh_parameters.push_back(base_name);
      }

      arg.setName(base_name);

      llvm::AllocaInst *alloca = bundle.builder.CreateAlloca(arg.getType(), nullptr, std::format("arg.{}", base_name));
      bundle.builder.CreateStore(&arg, alloca);

      bundle.env_llvm.vars[base_name] = alloca;

      name_idx += 1;
    }

    // Return setup
    if (this->body->block.scoped_return) {
      if (!return_type->isVoidTy()) {
        llvm::AllocaInst *r_alloca = bundle.builder.CreateAlloca(return_type, nullptr, "ret.val");
        bundle.env_llvm.return_alloca = r_alloca;
      }

      auto return_block = llvm::BasicBlock::Create(*bundle.context, "return");
      bundle.env_llvm.return_block = return_block;
    }
  }

  bundle.builder.SetInsertPoint(fn_body);

  // codegen the body
  this->body->codegen(bundle);

  // handle returns
  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      fn->insert(fn->end(), bundle.env_llvm.return_block);
      bundle.builder.SetInsertPoint(bundle.env_llvm.return_block);

      auto *return_value = bundle.builder.CreateLoad(return_type, bundle.env_llvm.return_alloca);
      bundle.builder.CreateRet(return_value);
    }
  } else if (return_type->isVoidTy()) {
    bundle.builder.CreateRetVoid();
  }

  // maintain the env
  bundle.env_llvm.return_block = outer_return_block;
  bundle.env_llvm.return_alloca = outer_return_alloca;

  //
  for (auto &shadowed : shadowed_parameters) {
    bundle.env_llvm.vars[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    bundle.env_llvm.vars.erase(fresh);
  }

  // TODO: Finish...
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*bundle.context), 2020);
}
