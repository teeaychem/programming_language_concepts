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

#include <vector>

#include "AST/Fmt.hpp"

using namespace llvm;

// Dec

// Code generation for a declaration.
// Should always be called when a declaration is made.
// The details of shadowing are handled at block nodes.
Value *AST::Dec::Var::codegen(LLVMBundle &hdl) const {
  auto existing = hdl.env_llvm.vars.find(this->name());
  if (existing != hdl.env_llvm.vars.end()) {
    return existing->second;
  }

  auto typ = this->typ->llvm(hdl);

  switch (this->typ->kind()) {

  case Typ::Kind::Ptr: {

    auto as_index = std::static_pointer_cast<Typ::Ptr>(this->typ);

    if (as_index->area().has_value()) {

      ArrayType *array_type = ArrayType::get(as_index->pointee_type()->llvm(hdl), as_index->area().value());

      switch (this->scope) {

      case Scope::Local: {

        auto size_value = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), as_index->area().value());
        auto alloca = hdl.builder.CreateAlloca(typ, size_value, this->name());
        hdl.env_llvm.vars[this->name()] = alloca;

        return alloca;
      } break;

      case Scope::Global: {

        auto alloca = hdl.module->getOrInsertGlobal(this->name(), array_type);
        GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
        ConstantAggregateZero *array_init = ConstantAggregateZero::get(array_type);
        globalVar->setInitializer(array_init);
        hdl.env_llvm.vars[this->name()] = globalVar;

        return alloca;

      } break;
      }
    }

    else {

      auto default_value = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(*hdl.context));

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
        hdl.builder.CreateStore(default_value, alloca);
        hdl.env_llvm.vars[this->name()] = alloca;

        return alloca;

      } break;

      case Scope::Global: {

        auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);
        GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
        globalVar->setInitializer(default_value);
        hdl.env_llvm.vars[this->name()] = globalVar;

        return alloca;

      } break;
      }
    }

  } break;

  case Typ::Kind::Int: {

    auto as_int = std::static_pointer_cast<AST::Typ::Int>(this->typ);
    auto default_value = as_int->defaultgen(hdl);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
      hdl.builder.CreateStore(default_value, alloca);
      hdl.env_llvm.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
      globalVar->setInitializer(default_value);
      hdl.env_llvm.vars[this->name()] = globalVar;

    } break;
    }

    return default_value;

  } break;

  case Typ::Kind::Char: {

    auto as_char = std::static_pointer_cast<AST::Typ::Char>(this->typ);
    auto default_value = as_char->defaultgen(hdl);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
      hdl.builder.CreateStore(default_value, alloca);
      hdl.env_llvm.vars[this->name()] = alloca;

    } break;

    case Scope::Global: {

      auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);
      GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
      globalVar->setInitializer(default_value);
      hdl.env_llvm.vars[this->name()] = globalVar;

    } break;
    }

    return default_value;
  }

  case Typ::Kind::Void: {
    throw std::logic_error("Creation of variable with void type");

  } break;
  }
}

// tmp

Value *AST::Dec::Prototype::codegen(LLVMBundle &hdl) const {
  if (hdl.env_llvm.vars.count(this->name()) != 0) {
    throw std::logic_error(std::format("Redeclaration of function: {}", this->name()));
  }

  llvm::Type *return_type = this->return_type()->llvm(hdl);
  std::vector<llvm::Type *> parameter_types{};

  { // Generate the parameter types
    parameter_types.reserve(this->args.size());

    for (auto &p : this->args) {
      parameter_types.push_back(p.second->llvm(hdl));
    }
  }

  auto fn_type = FunctionType::get(return_type, parameter_types, false);
  Function *fnx = Function::Create(fn_type, Function::ExternalLinkage, this->id, hdl.module.get());

  hdl.env_llvm.fns[this->id] = fnx;

  return fnx;
}

//

Value *AST::Dec::Fn::codegen(LLVMBundle &hdl) const {

  Function *fn = (Function *)this->prototype->codegen(hdl);

  // Fn details

  llvm::Type *return_type = this->return_type()->llvm(hdl);

  BasicBlock *outer_return_block = hdl.return_block; // stash any existing return block, to be restored on exit
  Value *outer_return_alloca = hdl.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  BasicBlock *fn_body = BasicBlock::Create(*hdl.context, "entry", fn);
  hdl.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->prototype->args[name_idx++].first;

      arg.setName(base_name);

      AllocaInst *alloca = hdl.builder.CreateAlloca(arg.getType(), nullptr, base_name);

      hdl.builder.CreateStore(&arg, alloca);

      auto it = hdl.env_llvm.vars.find(base_name);
      if (it != hdl.env_llvm.vars.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, alloca));
      } else {
        fresh_parameters.push_back(base_name);
      }

      hdl.env_llvm.vars[base_name] = alloca;
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
    hdl.env_llvm.vars[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    hdl.env_llvm.vars.erase(fresh);
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}
