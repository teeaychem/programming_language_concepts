
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

  std::cout << "Var: Checking env for " << this->var << " ... ";
  fflush(stdout);
  auto it = hdl.named_values.find(this->var);
  if (it == hdl.named_values.end()) {
    std::cerr << "Missing variable: " << this->var << std::endl;

    std::cout << "In scope:" << "\n";
    for (auto &x : hdl.named_values) {
      std::cout << "\t" << x.first << "\n";
    }

    exit(-1);
  }

  auto val = it->second;

  std::cout << "found ";
  fflush(stdout);

  return val;
}

// Expr

Value *AST::Expr::Access::codegen(LLVMBundle &hdl) {
  Value *r_val;

  switch (this->mode) {

  case Mode::Access: {
    auto value = this->acc->codegen(hdl);
    auto typ = this->acc->eval_type()->typegen(hdl);

    r_val = hdl.builder.CreateLoad(typ, value);

  } break;
  case Mode::Addr: {

    r_val = static_cast<AllocaInst *>(this->acc->codegen(hdl));

  } break;
  }

  return r_val;
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

  std::vector<std::pair<std::string, Value *>> shadowed_values{};

  bool stop = false;

  for (auto &fresh_dec : block.fresh_vars) {
    fresh_dec->codegen(hdl);
  }

  for (auto &shadow_dec : block.shadow_vars) {
    auto x = std::make_pair(shadow_dec->name(), hdl.named_values[shadow_dec->name()]);
    shadowed_values.push_back(x);

    shadow_dec->codegen(hdl);
  }

  for (auto &stmt : block.statements) {

    switch (stmt->kind()) {

    case AST::Stmt::Kind::Return: {
      stop = true;
    } break;

    default:
      break;
    }

    stmt->codegen(hdl);
  }

  // Unshadow shadowed vars
  for (auto &shadowed_var : shadowed_values) {
    hdl.named_values[shadowed_var.first] = shadowed_var.second;
  }

  // Clear fresh vars from scope
  for (auto &fresh_var : block.fresh_vars) {
    hdl.named_values.erase(fresh_var->name());
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::Expr::codegen(LLVMBundle &hdl) {
  return this->expr->codegen(hdl);
}

Value *AST::Stmt::If::codegen(LLVMBundle &hdl) {

  Function *parent = hdl.builder.GetInsertBlock()->getParent();

  BasicBlock *true_block = BasicBlock::Create(*hdl.context, "true", parent);
  BasicBlock *false_block = BasicBlock::Create(*hdl.context, "false");
  BasicBlock *merge_block = BasicBlock::Create(*hdl.context, "merge");

  Value *condition = this->condition->codegen(hdl);

  Value *zero = ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
  auto condition_eval = hdl.builder.CreateCmp(ICmpInst::ICMP_NE, condition, zero);

  hdl.builder.CreateCondBr(condition_eval, true_block, false_block);

  hdl.builder.SetInsertPoint(true_block);
  Value *true_eval = this->yes->codegen(hdl);



  hdl.builder.CreateBr(merge_block);
  // true_block = hdl.builder.GetInsertBlock(); // update for PHI

  parent->insert(parent->end(), false_block);
  hdl.builder.SetInsertPoint(false_block);
  Value *false_eval = this->no->codegen(hdl);

  hdl.builder.CreateBr(merge_block);
  // false_block = hdl.builder.GetInsertBlock();  // update for PHI

  parent->insert(parent->end(), merge_block);
  hdl.builder.SetInsertPoint(merge_block);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

Value *AST::Stmt::Return::codegen(LLVMBundle &hdl) {

  if (!hdl.return_block) {
    printf("Missing return block");
    std::exit(-1);
  }

  if (this->value.has_value()) {
    Value *r_dest = hdl.return_alloca;

    if (!r_dest) {
      printf("Return without destination");
      std::exit(-1);
    }

    Value *r_val = this->value.value()->codegen(hdl);

    hdl.builder.CreateStore(r_val, r_dest);
  }

  hdl.builder.CreateBr(hdl.return_block);

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Stmt::While::codegen(LLVMBundle &hdl) {
  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}

// Dec

// Code generation for a declaration.
// Should always be called when a declaration is made.
// The details of shadowing are handled at block nodes.
Value *AST::Dec::Var::codegen(LLVMBundle &hdl) {
  auto typ = this->typ->typegen(hdl);

  switch (this->scope) {

  case Scope::Local: {

    auto alloca = hdl.builder.CreateAlloca(typ, nullptr, this->name());
    hdl.named_values[this->name()] = alloca;
  } break;
  case Scope::Global: {

    auto alloca = hdl.module->getOrInsertGlobal(this->name(), typ);

    GlobalVariable *globalVar = hdl.module->getNamedGlobal(this->name());
    globalVar->setInitializer(this->typ->defaultgen(hdl));
    hdl.named_values[this->name()] = globalVar;
  } break;
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 0);
}

Value *AST::Dec::Fn::codegen(LLVMBundle &hdl) {
  BasicBlock *outer_return_block = hdl.return_block; // stash any existing return block, to be restored on exit
  Value *outer_return_alloca = hdl.return_alloca;    // likewise for return value allocation

  std::vector<std::pair<std::string, Value *>> shadowed_parameters{};
  std::vector<std::string> fresh_parameters{};

  llvm::Type *return_type = this->r_typ->typegen(hdl);
  std::vector<llvm::Type *> parameter_types{};

  { // Generate the parameter types
    parameter_types.reserve(this->params.size());

    for (auto &p : this->params) {
      parameter_types.push_back(p.first->typegen(hdl));
    }
  }

  auto fn_type = FunctionType::get(return_type, parameter_types, false);
  Function *fn = Function::Create(fn_type, Function::ExternalLinkage, this->id, hdl.module.get());

  BasicBlock *fn_body = BasicBlock::Create(*hdl.context, "entry", fn);
  hdl.builder.SetInsertPoint(fn_body);

  { // Parameters
    size_t name_idx{0};
    for (auto &arg : fn->args()) {

      auto &base_name = this->params[name_idx++].second;

      arg.setName(base_name);

      AllocaInst *alloca = create_fn_alloca(fn, base_name, arg.getType());
      hdl.builder.CreateStore(&arg, alloca);

      auto it = hdl.named_values.find(base_name);
      if (it != hdl.named_values.end()) {
        shadowed_parameters.push_back(std::make_pair(base_name, alloca));
      } else {
        fresh_parameters.push_back(base_name);
      }

      hdl.named_values[base_name] = alloca;
    }
  }

  // Return
  if (!return_type->isVoidTy()) {
    AllocaInst *r_alloca = create_fn_alloca(fn, "ret.val", return_type);
    hdl.return_alloca = r_alloca;
  }

  auto rb = BasicBlock::Create(*hdl.context, "return");
  hdl.return_block = rb;

  hdl.builder.SetInsertPoint(fn_body);

  this->body->codegen(hdl);

  fn->insert(fn->end(), rb);

  hdl.builder.SetInsertPoint(rb);

  if (!return_type->isVoidTy()) {
    auto *r_val = hdl.builder.CreateLoad(return_type, hdl.return_alloca);
    auto r_inst = hdl.builder.CreateRet(r_val);
  } else {
    auto r_inst = hdl.builder.CreateRetVoid();
  }

  hdl.return_block = outer_return_block;
  hdl.return_alloca = outer_return_alloca;

  for (auto &shadowed : shadowed_parameters) {
    hdl.named_values[shadowed.first] = shadowed.second;
  }

  for (auto &fresh : fresh_parameters) {
    hdl.named_values.erase(fresh);
  }

  return ConstantInt::get(Type::getInt64Ty(*hdl.context), 2020);
}
