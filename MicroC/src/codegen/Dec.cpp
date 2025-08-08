#include <vector>

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

#include "AST/Fmt.hpp"

using namespace llvm;

// Dec

// Code generation for a declaration.
// Should always be called when a declaration is made.
// The details of shadowing are handled at block nodes.
Value *AST::Dec::Var::codegen(LLVMBundle &bundle) const {
  auto existing = bundle.env_llvm.vars.find(this->name());
  if (existing != bundle.env_llvm.vars.end()) {
    return existing->second;
  }

  auto typ = this->typ->codegen(bundle);

  switch (this->typ->kind()) {

  case Typ::Kind::Ptr: {

    auto as_index = std::static_pointer_cast<Typ::Ptr>(this->typ);

    if (as_index->area().has_value()) {

      ArrayType *array_type = ArrayType::get(as_index->pointee_type()->codegen(bundle), as_index->area().value());

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = bundle.builder.CreateAlloca(typ, nullptr, this->name());
        bundle.env_llvm.vars[this->name()] = alloca;

        return alloca;
      } break;

      case Scope::Global: {

        auto alloca = bundle.module->getOrInsertGlobal(this->name(), array_type);
        GlobalVariable *globalVar = bundle.module->getNamedGlobal(this->name());
        ConstantAggregateZero *array_init = ConstantAggregateZero::get(array_type);
        globalVar->setInitializer(array_init);
        bundle.env_llvm.vars[this->name()] = globalVar;

        return alloca;

      } break;
      }
    }

    else {

      auto default_value = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*bundle.context));

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = bundle.builder.CreateAlloca(typ, nullptr, this->name());
        // bundle.builder.CreateStore(default_value, alloca);
        bundle.env_llvm.vars[this->name()] = alloca;

        return alloca;

      } break;

      case Scope::Global: {

        auto alloca = bundle.module->getOrInsertGlobal(this->name(), typ);
        GlobalVariable *globalVar = bundle.module->getNamedGlobal(this->name());
        globalVar->setInitializer(default_value);
        bundle.env_llvm.vars[this->name()] = globalVar;

        return alloca;

      } break;
      }
    }

  } break;

  case Typ::Kind::Int: {

    auto as_int = std::static_pointer_cast<AST::Typ::Int>(this->typ);
    auto default_value = as_int->defaultgen(bundle);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = bundle.builder.CreateAlloca(typ, nullptr, this->name());
      bundle.env_llvm.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = bundle.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = bundle.module->getNamedGlobal(this->name());
      globalVar->setInitializer(default_value);
      bundle.env_llvm.vars[this->name()] = globalVar;

    } break;
    }

    return default_value;

  } break;

  case Typ::Kind::Char: {

    auto as_char = std::static_pointer_cast<AST::Typ::Char>(this->typ);
    auto default_value = as_char->defaultgen(bundle);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = bundle.builder.CreateAlloca(typ, nullptr, this->name());
      bundle.env_llvm.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = bundle.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = bundle.module->getNamedGlobal(this->name());
      globalVar->setInitializer(default_value);
      bundle.env_llvm.vars[this->name()] = globalVar;

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

// tmp

Value *AST::Dec::Prototype::codegen(LLVMBundle &bundle) const {
  llvm::Type *return_type = this->return_type()->codegen(bundle);
  std::vector<llvm::Type *> parameter_types{};

  // Generate the parameter types
  parameter_types.reserve(this->args.size());
  for (auto &p : this->args) {
    parameter_types.push_back(p.second->codegen(bundle));
  }

  auto fn_type = FunctionType::get(return_type, parameter_types, false);
  Function *fn = Function::Create(fn_type, Function::ExternalLinkage, this->id, bundle.module.get());

  bundle.env_llvm.fns[this->id] = fn;

  return fn;
}

//

Value *AST::Dec::Fn::codegen(LLVMBundle &bundle) const {

  Function *fn = (Function *)this->prototype->codegen(bundle);

  // Fn details

  llvm::Type *return_type = this->return_type()->codegen(bundle);

  BasicBlock *outer_return_block = bundle.return_block; // to be restored on exit
  Value *outer_return_alloca = bundle.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  BasicBlock *fn_body = BasicBlock::Create(*bundle.context, "entry", fn);
  bundle.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->prototype->args[name_idx].first;
      auto &base_type = this->prototype->args[name_idx].second;
      name_idx += 1;

      auto it = bundle.env_llvm.vars.find(base_name);
      if (it != bundle.env_llvm.vars.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, it->second));
      } else {
        fresh_parameters.push_back(base_name);
      }

      arg.setName(base_name);

      AllocaInst *alloca = bundle.builder.CreateAlloca(arg.getType(), nullptr, std::format("arg.{}", base_name));
      bundle.builder.CreateStore(&arg, alloca);
      bundle.env_llvm.vars[base_name] = alloca;
    }

    // Return setup
    if (this->body->block.scoped_return) {
      if (!return_type->isVoidTy()) {
        AllocaInst *r_alloca = bundle.builder.CreateAlloca(return_type, nullptr, "ret.val");
        bundle.return_alloca = r_alloca;
      }

      auto return_block = BasicBlock::Create(*bundle.context, "return");
      bundle.return_block = return_block;
    }
  }

  bundle.builder.SetInsertPoint(fn_body);

  this->body->codegen(bundle);

  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      fn->insert(fn->end(), bundle.return_block);
      bundle.builder.SetInsertPoint(bundle.return_block);

      auto *return_value = bundle.builder.CreateLoad(return_type, bundle.return_alloca);
      bundle.builder.CreateRet(return_value);
    }
  } else if (return_type->isVoidTy()) {
    bundle.builder.CreateRetVoid();
  }

  bundle.return_block = outer_return_block;
  bundle.return_alloca = outer_return_alloca;

  for (auto &shadowed : shadowed_parameters) {
    bundle.env_llvm.vars[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    bundle.env_llvm.vars.erase(fresh);
  }

  // TODO: Finish...
  return ConstantInt::get(Type::getInt64Ty(*bundle.context), 2020);
}
