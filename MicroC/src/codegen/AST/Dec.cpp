#include <format>
#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"
#include "codegen/Structs.hpp"

// Dec

// Prototype
// Return the fn as the implementation doesn't permit prototypes independent of a body declaration.
// If revised to do so, the fn generation should be abstracted as this is called during fn declaration.
llvm::Value *AST::Dec::Prototype::codegen(Context &ctx) const {
  llvm::Type *return_type = this->return_type()->codegen(ctx);

  std::vector<llvm::Type *> arg_types{};
  arg_types.reserve(this->args.size());
  for (auto &arg : this->args) {
    arg_types.push_back(arg.typ->codegen(ctx));
  }

  auto fn_type = llvm::FunctionType::get(return_type, arg_types, false);
  llvm::Function *fn = llvm::Function::Create(fn_type,
                                              llvm::Function::ExternalLinkage,
                                              this->id,
                                              ctx.module.get());

  ctx.env_llvm.fns[this->id] = fn;

  return fn;
}

// Fn
// The fn codegen is sourced to prototype codegen, though see comments for issues with this.
// The codegen given is for the body.
// This amounts to setting up local allocas for each arg, generating the body, and maintaining the env.
//
// Returns are handled by setting `return_alloca` in the ctx to the local return alloca.
// This helps simplify control structure, as any return makes a store and then (something equivalent to) a jump to the end of the fn.
llvm::Value *AST::Dec::Fn::codegen(Context &ctx) const {

  llvm::Function *fn = (llvm::Function *)this->prototype->codegen(ctx);

  // Fn details

  llvm::Type *return_type = this->return_type()->codegen(ctx);

  llvm::BasicBlock *outer_return_block = ctx.env_llvm.return_block; // to be restored on exit
  llvm::Value *outer_return_alloca = ctx.env_llvm.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, llvm::Value *>> shadowed_args{};
  std::vector<std::string> fresh_args{};

  llvm::BasicBlock *fn_body = llvm::BasicBlock::Create(*ctx.context, "entry", fn);
  ctx.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t arg_idx{0};
    for (auto &arg : fn->args()) {

      auto &arg_var = this->prototype->args[arg_idx].var;
      auto &arg_typ = this->prototype->args[arg_idx].typ;

      auto it = ctx.env_llvm.vars.find(arg_var);
      if (it != ctx.env_llvm.vars.end()) {
        shadowed_args.push_back(std::make_pair(arg_var, it->second));
      } else {
        fresh_args.push_back(arg_var);
      }

      arg.setName(arg_var);

      llvm::AllocaInst *alloca = ctx.builder.CreateAlloca(arg.getType(),
                                                          nullptr,
                                                          std::format("arg.{}", arg_var));
      ctx.builder.CreateStore(&arg, alloca);

      ctx.env_llvm.vars[arg_var] = alloca;

      arg_idx += 1;
    }

    // Return setup
    if (this->body->block.scoped_return) {
      if (!return_type->isVoidTy()) {
        llvm::AllocaInst *r_alloca = ctx.builder.CreateAlloca(return_type, nullptr, "ret.val");
        ctx.env_llvm.return_alloca = r_alloca;
      }

      auto return_block = llvm::BasicBlock::Create(*ctx.context, "return");
      ctx.env_llvm.return_block = return_block;
    }
  }

  ctx.builder.SetInsertPoint(fn_body);

  // codegen the body
  this->body->codegen(ctx);

  // handle returns
  if (this->body->block.scoped_return) {
    if (!return_type->isVoidTy()) {
      fn->insert(fn->end(), ctx.env_llvm.return_block);
      ctx.builder.SetInsertPoint(ctx.env_llvm.return_block);

      auto *return_val = ctx.builder.CreateLoad(return_type, ctx.env_llvm.return_alloca);
      ctx.builder.CreateRet(return_val);
    }
  } else if (return_type->isVoidTy()) {
    ctx.builder.CreateRetVoid();
  }

  // maintain the env
  ctx.env_llvm.return_block = outer_return_block;
  ctx.env_llvm.return_alloca = outer_return_alloca;

  //
  for (auto &shadowed : shadowed_args) {
    ctx.env_llvm.vars[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_args) {
    ctx.env_llvm.vars.erase(fresh);
  }

  // TODO: Finish...
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*ctx.context), 2020);
}

llvm::Value *AST::Dec::Var::codegen(Context &ctx) const {
  auto existing = ctx.env_llvm.vars.find(this->var());

  auto typ = this->_typ->codegen(ctx);
  auto var = this->var();

  // codegen splits for each type, and in turn splits on local / global.
  // the type split is used to generate default values to assign to global and to use as return values.
  // defalt values are not stored for local declarations, in line with C.
  // (Though, mostly to reduce the amount of IR generated.)
  switch (this->_typ->kind()) {

  case Typ::Kind::Ptr: {

    auto as_ptr = std::static_pointer_cast<Typ::Ptr>(this->_typ);
    auto ptr = llvm::PointerType::getUnqual(*ctx.context);
    auto default_val = llvm::ConstantPointerNull::get(ptr);

    // Pointers split again on whether the pointer has some associated area.
    // That is, on a whether the pointer is (explicitly) to an array or not.
    if (as_ptr->area().has_value()) {

      llvm::ArrayType *array_typ = llvm::ArrayType::get(as_ptr->pointee_typ()->codegen(ctx),
                                                        as_ptr->area().value());

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = ctx.builder.CreateAlloca(typ, nullptr, var); // Create
        ctx.env_llvm.vars[var] = alloca;                           // Update env

      } break;

      case Scope::Global: {

        ctx.module->getOrInsertGlobal(var, array_typ);                                   // Create
        llvm::GlobalVariable *globalVar = ctx.module->getNamedGlobal(var);               // Find
        llvm::ConstantAggregateZero *init = llvm::ConstantAggregateZero::get(array_typ); // Init a.
        globalVar->setInitializer(init);                                                 // Init b.
        ctx.env_llvm.vars[var] = globalVar;                                              // Update env

      } break;
      }

      return default_val;
    }

    else {

      switch (this->scope) {

      case Scope::Local: {

        auto alloca = ctx.builder.CreateAlloca(typ, nullptr, var);
        ctx.env_llvm.vars[var] = alloca;

      } break;

      case Scope::Global: {

        ctx.module->getOrInsertGlobal(var, typ);
        llvm::GlobalVariable *globalVar = ctx.module->getNamedGlobal(var);
        globalVar->setInitializer(default_val);
        ctx.env_llvm.vars[var] = globalVar;

      } break;
      }

      return default_val;
    }

  } break;

  case Typ::Kind::Int: {

    auto as_int = std::static_pointer_cast<AST::Typ::Int>(this->_typ);
    auto default_val = as_int->defaultgen(ctx);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = ctx.builder.CreateAlloca(typ, nullptr, var);
      ctx.env_llvm.vars[var] = alloca;

    } break;

    case Scope::Global: {

      ctx.module->getOrInsertGlobal(var, typ);
      llvm::GlobalVariable *globalVar = ctx.module->getNamedGlobal(var);
      globalVar->setInitializer(default_val);
      ctx.env_llvm.vars[var] = globalVar;

    } break;
    }

    return default_val;

  } break;

  case Typ::Kind::Char: {

    auto as_char = std::static_pointer_cast<AST::Typ::Char>(this->_typ);
    auto default_val = as_char->defaultgen(ctx);

    switch (this->scope) {

    case Scope::Local: {

      auto alloca = ctx.builder.CreateAlloca(typ, nullptr, var);
      ctx.env_llvm.vars[var] = alloca;

    } break;

    case Scope::Global: {

      ctx.module->getOrInsertGlobal(var, typ);
      llvm::GlobalVariable *globalVar = ctx.module->getNamedGlobal(var);
      globalVar->setInitializer(default_val);
      ctx.env_llvm.vars[var] = globalVar;

    } break;
    }

    return default_val;
  }

  case Typ::Kind::Void: {
    throw std::logic_error("Creation of variable with void type");
  } break;

  case Typ::Kind::Bool: {
    throw std::logic_error("Creation of variable with bool type");
  } break;
  }
}
