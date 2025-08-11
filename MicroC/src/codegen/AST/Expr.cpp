#include <vector>
#include <format>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"
#include "codegen/Structs.hpp"

using namespace llvm;

Value *AST::Expr::Var::codegen(Context &ctx) const {
  auto it = ctx.env_llvm.vars.find(this->var);
  if (it == ctx.env_llvm.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  return it->second;
}

Value *AST::Expr::Call::codegen(Context &ctx) const {

  Function *callee_f = ctx.module->getFunction(this->name);
  auto prototype = ctx.env_ast.fns.find(this->name)->second.get();

  if (callee_f == nullptr) {
    auto it = ctx.foundation_fn_map.find(this->name);
    if (it != ctx.foundation_fn_map.end()) {
      callee_f = it->second->codegen(ctx);
    } else {
      throw std::logic_error(std::format("Call to unknown fn: {}", this->name));
    }
  } else if (callee_f->arg_size() != this->arguments.size()) {
    throw std::logic_error(std::format("Call to {} requires {} arguments, received {}.",
                                       this->name, callee_f->arg_size(), this->arguments.size()));
  }

  std::vector<Value *> arg_values{};
  for (size_t i = 0; i < this->arguments.size(); ++i) {

    auto [access_val, access_typ] = ctx.access(this->arguments[i].get());
    arg_values.push_back(access_val);
  }

  // Named instructions cannot provide void values
  return ctx.builder.CreateCall(callee_f, arg_values);
}

Value *AST::Expr::Cast::codegen(Context &ctx) const {

  switch (this->type_kind()) {

  case Typ::Kind::Bool:
  case Typ::Kind::Char:
  case Typ::Kind::Void: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;

  case Typ::Kind::Int: {

    switch (expr->type_kind()) {

    case Typ::Kind::Bool: {
      auto cast = ctx.builder.CreateIntCast(this->expr->codegen(ctx),
                                            this->type()->codegen(ctx), false);
      return cast;
    } break;

    case Typ::Kind::Char:
    case Typ::Kind::Int:
    case Typ::Kind::Void: {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    } break;

    case Typ::Kind::Ptr: {
      auto cast = ctx.builder.CreatePtrToInt(this->expr->codegen(ctx),
                                             this->type()->codegen(ctx));
      return cast;
    } break;
    }

  } break;

  case Typ::Kind::Ptr: {
    if (this->expr->typ_has_kind(AST::Typ::Kind::Int)) {
      auto cast = ctx.builder.CreateIntToPtr(this->expr->codegen(ctx),
                                             this->type()->codegen(ctx));
      return cast;
    } else {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    }
  } break;
  }
}

Value *AST::Expr::CstI::codegen(Context &ctx) const {
  return ConstantInt::get(this->type()->codegen(ctx), this->i, true);
}

Value *AST::Expr::Index::codegen(Context &ctx) const {

  auto [ptr_val, ptr_typ] = ctx.access(this->target.get());
  auto [access_val, access_typ] = ctx.access(this->index.get());

  auto ptr_add = ctx.builder.CreateInBoundsGEP(ptr_typ->deref()->codegen(ctx),
                                               ptr_val,
                                               ArrayRef<Value *>{access_val},
                                               "idx");

  return ptr_add;
}

Value *AST::Expr::Prim1::codegen(Context &ctx) const {

  llvm::Value *return_value;

  switch (this->op) {

  case OpUnary::AddressOf: {
    // The address of a dereference is obtained by avoiding the dereference.
    // This is an instance of the general approach to never take the address of an object.
    return_value = expr->codegen(ctx);

  } break;

  case OpUnary::Dereference: {
    // Dereference performs a load / access.
    auto [access_val, _] = ctx.access(this->expr.get());
    return_value = access_val;
  } break;

  case OpUnary::Sub: {
    auto [access_val, _] = ctx.access(expr.get());
    return_value = ctx.builder.CreateNeg(access_val, "op.neg");
  } break;

  case OpUnary::Negation: {
    auto [access_val, _] = ctx.access(expr.get());
    return_value = ctx.builder.CreateNot(access_val, "op.not");
  } break;
  }

  return return_value;
}

// Support

Value *AST::ExprT::codegen_eval_true(Context &ctx) const {

  llvm::Value *return_value;

  auto [access_val, access_typ] = ctx.access(this);

  // If Already a boolean...
  if (access_val->getType()->isIntegerTy(1)) {
    return_value = access_val;
  }
  // Otherwise test not equal to null val of expr type
  else {
    return_value = ctx.builder.CreateCmp(ICmpInst::ICMP_NE,
                                         access_val,
                                         access_typ->defaultgen(ctx),
                                         "op.eval_true");
  }

  return return_value;
}

Value *AST::ExprT::codegen_eval_false(Context &ctx) const {

  llvm::Value *return_value;

  auto [access_val, access_typ] = ctx.access(this);

  if (access_val->getType()->isIntegerTy(1)) {
    return_value = access_val;
  } else {
    return_value = ctx.builder.CreateCmp(ICmpInst::ICMP_EQ,
                                         access_val,
                                         access_typ->defaultgen(ctx),
                                         "op.eval_false");
  }

  return return_value;
}

// Binary ops

namespace OpBinaryCodegen {

llvm::Value *builder_assign(Context &ctx, AST::ExprHandle destination, AST::ExprHandle value) {

  llvm::Value *destination_val = destination->codegen(ctx);

  auto [access_val, access_typ] = ctx.access(value.get());
  ctx.builder.CreateStore(access_val, destination_val, "op.assign");

  return access_val;
}

// Codegen for ptr + int
llvm::Value *builder_ptr_add(Context &ctx, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto [access_val, access_typ] = ctx.access(val.get());
  auto [ptr_val, ptr_typ] = ctx.access(ptr.get());
  auto ptr_add = ctx.builder.CreateInBoundsGEP(ptr_typ->deref()->codegen(ctx),
                                               ptr_val,
                                               ArrayRef<Value *>{access_val},
                                               "add.ptr");

  return ptr_add;
}

llvm::Value *builder_add(Context &ctx, const AST::Expr::Prim2 *expr) {

  llvm::Value *return_value;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto [lhs_val, lhs_typ] = ctx.access(expr->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(expr->rhs.get());

    return_value = ctx.builder.CreateAdd(lhs_val, rhs_val, "op.add");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      return_value = builder_ptr_add(ctx, expr->lhs, expr->rhs);
    }

    else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      return_value = builder_ptr_add(ctx, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  } break;
  }

  return return_value;
}

// Codegen for ptr - int
llvm::Value *builder_ptr_sub(Context &ctx, AST::ExprHandle ptr, AST::ExprHandle val) {

  auto [access_val, access_typ] = ctx.access(val.get());

  auto ptr_typ = ptr->type()->codegen(ctx);
  auto ptr_elem = ctx.builder.CreateNeg(access_val);
  auto ptr_sub = ctx.builder.CreateGEP(ptr_typ,
                                       ptr->codegen(ctx),
                                       ArrayRef<Value *>(ptr_elem),
                                       "sub");

  return ptr_sub;
}

llvm::Value *builder_sub(Context &ctx, const AST::Expr::Prim2 *expr) {

  llvm::Value *return_value;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto [lhs_val, lhs_typ] = ctx.access(expr->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(expr->rhs.get());

    return_value = ctx.builder.CreateSub(lhs_val, rhs_val, "op.sub");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      return_value = builder_ptr_sub(ctx, expr->lhs, expr->rhs);
    }

    else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      return_value = builder_ptr_sub(ctx, expr->rhs, expr->lhs);
    }

    else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }
  }

  break;
  }

  return return_value;
}

llvm::Value *builder_mul(Context &ctx, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = ctx.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = ctx.access(expr->rhs.get());

  return ctx.builder.CreateMul(lhs_val, rhs_val, "op.mul");
}

llvm::Value *builder_div(Context &ctx, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = ctx.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = ctx.access(expr->rhs.get());

  return ctx.builder.CreateSDiv(lhs_val, rhs_val, "op.div");
}

llvm::Value *builder_mod(Context &ctx, const AST::Expr::Prim2 *expr) {
  auto [lhs_val, lhs_typ] = ctx.access(expr->lhs.get());
  auto [rhs_val, rhs_typ] = ctx.access(expr->rhs.get());

  return ctx.builder.CreateSRem(lhs_val, rhs_val, "op.mod");
}

} // namespace OpBinaryCodegen

Value *AST::Expr::Prim2::codegen(Context &ctx) const {

  switch (op) {

  case OpBinary::Assign: {
    return OpBinaryCodegen::builder_assign(ctx, this->lhs, this->rhs);
  } break;

  case OpBinary::AssignAdd: {
    throw std::logic_error("todo: +=");
  } break;

  case OpBinary::AssignSub: {
    throw std::logic_error("todo: -=");
  } break;

  case OpBinary::AssignMul: {
    throw std::logic_error("todo: *=");
  } break;

  case OpBinary::AssignDiv: {
    throw std::logic_error("todo: /=");
  } break;

  case OpBinary::AssignMod: {
    throw std::logic_error("todo: %=");
  } break;

  case OpBinary::Add: {
    return OpBinaryCodegen::builder_add(ctx, this);
  } break;

  case OpBinary::Sub: {
    return OpBinaryCodegen::builder_sub(ctx, this);
  } break;

  case OpBinary::Mul: {
    return OpBinaryCodegen::builder_mul(ctx, this);
  } break;

  case OpBinary::Div: {
    return OpBinaryCodegen::builder_div(ctx, this);
  } break;

  case OpBinary::Mod: {
    return OpBinaryCodegen::builder_mod(ctx, this);
  } break;

  case OpBinary::Eq: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, lhs_val, rhs_val, "op.eq");
  } break;

  case OpBinary::Neq: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, lhs_val, rhs_val, "op.neq");
  } break;

  case OpBinary::Gt: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, lhs_val, rhs_val, "op.gt");
  } break;

  case OpBinary::Lt: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, lhs_val, rhs_val, "op.lt");
  } break;

  case OpBinary::Leq: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, lhs_val, rhs_val, "op.leq");
  } break;

  case OpBinary::Geq: {
    auto [lhs_val, lhs_typ] = ctx.access(this->lhs.get());
    auto [rhs_val, rhs_typ] = ctx.access(this->rhs.get());

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SGE, lhs_val, rhs_val, "op.geq");
  } break;

  case OpBinary::And: {
    auto lhs_val = this->lhs->codegen_eval_true(ctx);
    auto rhs_val = this->rhs->codegen_eval_true(ctx);

    return ctx.builder.CreateAnd(lhs_val, rhs_val, "op.and");
  } break;

  case OpBinary::Or: {
    auto lhs_val = this->lhs->codegen_eval_true(ctx);
    auto rhs_val = this->rhs->codegen_eval_true(ctx);

    return ctx.builder.CreateOr(lhs_val, rhs_val, "op.or");
  } break;
  }
}
