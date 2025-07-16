
#include "CodegenLLVM.hh"
#include "AST/AST.hh"

#include "AST/Node/Access.hh"
#include "AST/Node/Dec.hh"
#include "AST/Node/Expr.hh"
#include "AST/Node/Stmt.hh"

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

using namespace llvm;

// Support

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
static AllocaInst *create_fn_alloca(Function *fn, StringRef name, Type *typ) {
  IRBuilder<> TmpB(&fn->getEntryBlock(), fn->getEntryBlock().begin());
  return TmpB.CreateAlloca(typ, nullptr, name);
}

// Access

Value *AST::Access::Deref::codegen(LLVMBundle &hdl) {
  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

Value *AST::Access::Index::codegen(LLVMBundle &hdl) {
  Value *value = this->access->codegen(hdl);
  Value *index = this->index->codegen(hdl);

  auto ptr = hdl.builder.CreateGEP(Type::getInt64Ty(*hdl.context), value, ArrayRef<Value *>(index));

  return ptr;
}

Value *AST::Access::Var::codegen(LLVMBundle &hdl) {

  auto it = hdl.named_values.find(this->var);
  if (it == hdl.named_values.end()) {
    std::cerr << "Missing variable: " << this->var << "\n";
    exit(-1);
  }

  return it->second;
}

// Expr

Value *AST::Expr::Access::codegen(LLVMBundle &hdl) {
  switch (this->mode) {

  case Mode::Access: {
    auto x = static_cast<AllocaInst *>(this->acc->codegen(hdl));

    auto *r_val = hdl.builder.CreateLoad(x->getAllocatedType(), x);

    return r_val;
  } break;
  case Mode::Addr: {

    auto x = static_cast<AllocaInst *>(this->acc->codegen(hdl));

    return x;
  } break;
  }
}

Value *AST::Expr::Assign::codegen(LLVMBundle &hdl) {

  Value *value = this->expr->codegen(hdl);
  hdl.builder.CreateStore(value, this->dest->codegen(hdl));

  return value;
}

Value *AST::Expr::Call::codegen(LLVMBundle &hdl) {
  Function *callee_f = hdl.module->getFunction(this->name);

  std::vector<Value *> arg_vs{};

  if (callee_f != nullptr) {
    if (callee_f->arg_size() != this->parameters.size()) {
      std::cerr << "Argument size mismatch." << "\n";
    }
  } else {

    if (this->name == "printi") {
      callee_f = hdl.base_fns["printf"];
      arg_vs.push_back(hdl.builder.CreateGlobalString("%d\n"));
    } else {
      std::cerr << "Call to unknown function: " << this->name << "\n";
      exit(-1);
    }
  }

  for (auto &arg : this->parameters) {
    auto arg_v = arg->codegen(hdl);
    if (arg_v == nullptr) {
      std::cerr << "Failed to process argument " << arg->to_string(0) << "\n";
      std::exit(-1);
    }

    arg_vs.push_back(arg_v);
  }

  return hdl.builder.CreateCall(callee_f, arg_vs);
}

Value *AST::Expr::CstI::codegen(LLVMBundle &hdl) {
  return ConstantInt::get(llvm::Type::getInt64Ty(*hdl.context), this->i, true);
}

Value *AST::Expr::Prim1::codegen(LLVMBundle &hdl) {

  Value *expr_val = this->expr->codegen(hdl);

  if (hdl.prim1_fns[this->op]) {
    return hdl.prim1_fns[this->op](expr_val);
  } else {
    std::cerr << "Unexpected unary op: " << this->op << "\n";
    std::exit(-1);
  }
}

Value *AST::Expr::Prim2::codegen(LLVMBundle &hdl) {

  Value *a_val = this->a->codegen(hdl);
  Value *b_val = this->b->codegen(hdl);

  if (hdl.prim2_fns[this->op]) {
    return hdl.prim2_fns[this->op](a_val, b_val);
  } else {
    std::cerr << "Unexpected binary op: " << this->op << "\n";
    std::exit(-1);
  }
}

// Stmt

Value *AST::Stmt::Block::codegen(LLVMBundle &hdl) {

  // Function *TheFunction = hdl.Builder.GetInsertBlock()->getParent();
  // BasicBlock *BlockBB = BasicBlock::Create(*hdl.Context, "block", TheFunction);

  // hdl.Builder.SetInsertPoint(BlockBB);

  bool stop = false;

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(hdl);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    shadow_dec->codegen(hdl);
  }

  for (auto &stmt : block.statements) {

    switch (stmt->kind()) {

    case AST::Stmt::Kind::Return: {
      stmt->codegen(hdl);
      stop = true;
    } break;

    case AST::Stmt::Kind::Expr: {
      stmt->codegen(hdl);
    } break;

    default:
      stmt->codegen(hdl);
    }
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::Expr::codegen(LLVMBundle &hdl) {
  return this->expr->codegen(hdl);
}

Value *AST::Stmt::If::codegen(LLVMBundle &hdl) {
  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

Value *AST::Stmt::Return::codegen(LLVMBundle &hdl) {

  if (!hdl.named_blocks["return"]) {
    printf("Missing return block");
    std::exit(-1);
  }

  if (this->value.has_value()) {
    Value *r_dest = hdl.named_values["ret.val"];

    if (!r_dest) {
      printf("Return without destination");
      std::exit(-1);
    }
    Value *r_val = this->value.value()->codegen(hdl);

    hdl.builder.CreateStore(r_val, r_dest);
  }

  hdl.builder.CreateBr(hdl.named_blocks["return"]);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::While::codegen(LLVMBundle &hdl) {
  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

// Dec

Value *AST::Dec::Var::codegen(LLVMBundle &hdl) {
  switch (this->scope) {

  case Scope::Local: {
    auto typ = this->typ->typegen(hdl);
    auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->id);
    hdl.named_values[this->id] = alloca;
  } break;
  case Scope::Global: {
    auto x = hdl.module->getOrInsertGlobal(this->id, this->typ->typegen(hdl));
  } break;
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Dec::Fn::codegen(LLVMBundle &hdl) {

  hdl.named_values.clear(); // Functions are top level

  llvm::Type *r_typ = this->r_typ->typegen(hdl);
  std::vector<llvm::Type *> param_typs{};

  { // Generate the parameter types
    param_typs.reserve(this->params.size());

    for (auto &p : this->params) {
      param_typs.push_back(p.first->typegen(hdl));
    }
  }

  auto fn_typ = FunctionType::get(r_typ, param_typs, false);
  Function *fn = Function::Create(fn_typ, Function::ExternalLinkage, this->id, hdl.module.get());

  BasicBlock *fn_body = BasicBlock::Create(*hdl.context, std::format("entry", this->id), fn);
  hdl.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->params[name_idx++].second;

      arg.setName(base_name);

      std::string name = base_name;

      AllocaInst *alloca = create_fn_alloca(fn, name, arg.getType());
      hdl.builder.CreateStore(&arg, alloca);

      hdl.named_values[name] = alloca;
    }
  }

  { // Return
    if (!r_typ->isVoidTy()) {
      std::string r_name = "ret.val";
      AllocaInst *r_alloca = create_fn_alloca(fn, r_name, r_typ);
      hdl.named_values[r_name] = r_alloca;
    }

    auto rb = BasicBlock::Create(*hdl.context, "return", fn);
    hdl.named_blocks["return"] = rb;
    hdl.builder.SetInsertPoint(rb);

    if (!r_typ->isVoidTy()) {
      auto r_alloca = hdl.named_values["ret.val"];
      auto *r_val = hdl.builder.CreateLoad(r_typ, r_alloca);
      auto r_inst = hdl.builder.CreateRet(r_val);
    } else {
      auto r_inst = hdl.builder.CreateRetVoid();
    }
  }

  hdl.builder.SetInsertPoint(fn_body);

  this->body->codegen(hdl);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}
