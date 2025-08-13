#include <cstdio>
#include <format>
#include <memory>
#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"
#include "codegen/Structs.hpp"

llvm::Value *AST::Expr::Var::codegen(Context &ctx, AST::Expr::Value value) const {
  auto find_var = ctx.env_llvm.vars.find(this->var);
  if (find_var == ctx.env_llvm.vars.end()) {
    throw std::logic_error(std::format("Missing variable: {}", this->var));
  }

  auto val = find_var->second;

  if (value == AST::Expr::Value::R) {
    switch (this->type()->kind()) {

    case Typ::Kind::Bool:
    case Typ::Kind::Char:
    case Typ::Kind::Int: {
      val = ctx.builder.CreateLoad(this->type()->codegen(ctx), val);
    } break;

    case Typ::Kind::Ptr: {
      auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(this->type());
      if (ptr_typ->area().has_value()) {
        val = ctx.builder.CreateInBoundsGEP(ptr_typ->codegen(ctx),
                                            val,
                                            llvm::ArrayRef{ctx.get_zero(), ctx.get_zero()},
                                            "decayR");

      } else {
        val = ctx.builder.CreateLoad(this->type()->codegen(ctx), val);
      }

    } break;

    case Typ::Kind::Void: {
    } break;
    }
  }

  return val;
}

llvm::Value *AST::Expr::Call::codegen(Context &ctx, AST::Expr::Value value) const {

  llvm::Function *callee_f = ctx.module->getFunction(this->name);
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

  std::vector<llvm::Value *> arg_vals{};
  for (auto arg : this->arguments) {
    auto arg_val = arg->codegen(ctx, AST::Expr::Value::R);
    arg_vals.push_back(arg_val);
  }

  // Named instructions cannot provide void values
  return ctx.builder.CreateCall(callee_f, arg_vals);
}

llvm::Value *AST::Expr::Cast::codegen(Context &ctx, AST::Expr::Value value) const {

  switch (this->type_kind()) {

  case Typ::Kind::Bool:
  case Typ::Kind::Char:
  case Typ::Kind::Void: {
    throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
  } break;

  case Typ::Kind::Int: {

    switch (expr->type_kind()) {

    case Typ::Kind::Bool: {
      auto cast = ctx.builder.CreateIntCast(this->expr->codegen(ctx, value),
                                            this->type()->codegen(ctx),
                                            false,
                                            "cst");
      return cast;
    } break;

    case Typ::Kind::Char:
    case Typ::Kind::Int:
    case Typ::Kind::Void: {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    } break;

    case Typ::Kind::Ptr: {

      auto cast = ctx.builder.CreatePtrToInt(this->expr->codegen(ctx, value),
                                             this->type()->codegen(ctx), "cst");
      return cast;
    } break;
    }

  } break;

  case Typ::Kind::Ptr: {
    if (this->expr->typ_has_kind(AST::Typ::Kind::Int)) {
      auto cast = ctx.builder.CreateIntToPtr(this->expr->codegen(ctx, value),
                                             this->type()->codegen(ctx));
      return cast;
    } else {
      throw std::logic_error(std::format("Unsupported cast {}", this->to_string()));
    }
  } break;
  }
}

llvm::Value *AST::Expr::CstI::codegen(Context &ctx, AST::Expr::Value value) const {
  return llvm::ConstantInt::get(this->type()->codegen(ctx), this->i, true);
}

llvm::Value *AST::Expr::Prim1::codegen(Context &ctx, AST::Expr::Value value) const {

  llvm::Value *val;

  switch (this->op) {

  case OpUnary::AddressOf: {
    // The address of a dereference is obtained by avoiding the dereference.
    // This is an instance of the general approach to never take the address of an object.
    val = expr->codegen(ctx, AST::Expr::Value::L);
  } break;

  case OpUnary::Dereference: {
    auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(expr->type());

    val = this->expr->codegen(ctx, AST::Expr::Value::R);

    if (value == AST::Expr::Value::R) {

      switch (this->type()->kind()) {

      case Typ::Kind::Bool:
      case Typ::Kind::Char:
      case Typ::Kind::Int: {
        val = ctx.builder.CreateLoad(this->type()->codegen(ctx), val);
      } break;

      case Typ::Kind::Ptr: {
        auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(this->type());
        if (ptr_typ->area().has_value()) {
          val = ctx.builder.CreateInBoundsGEP(ptr_typ->codegen(ctx),
                                              val,
                                              llvm::ArrayRef{ctx.get_zero(), ctx.get_zero()},
                                              "decayR");

        } else {
          val = ctx.builder.CreateLoad(this->type()->codegen(ctx), val);
        }

      } break;

      case Typ::Kind::Void: {
      } break;
      }
    }

  } break;

  case OpUnary::Sub: {
    val = expr->codegen(ctx, AST::Expr::Value::R);
    val = ctx.builder.CreateNeg(val, "op.neg");
  } break;

  case OpUnary::Negation: {
    val = expr->codegen(ctx, AST::Expr::Value::R);
    val = ctx.builder.CreateNot(val, "op.not");
  } break;
  }

  return val;
}

// Support

llvm::Value *AST::ExprT::codegen_eval_true(Context &ctx) const {

  llvm::Value *return_value;

  auto val = this->codegen(ctx, AST::Expr::Value::R);

  // If Already a boolean...
  if (val->getType()->isIntegerTy(1)) {
    return_value = val;
  }
  // Otherwise test not equal to null val of expr type
  else {
    return_value = ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_NE,
                                         val,
                                         this->type()->defaultgen(ctx),
                                         "op.eval_true");
  }

  return return_value;
}

llvm::Value *AST::ExprT::codegen_eval_false(Context &ctx) const {

  llvm::Value *return_value;

  auto val = this->codegen(ctx, AST::Expr::Value::R);

  if (val->getType()->isIntegerTy(1)) {
    return_value = val;
  } else {
    return_value = ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ,
                                         val,
                                         this->type()->defaultgen(ctx),
                                         "op.eval_false");
  }

  return return_value;
}

// Binary ops

namespace OpBinaryCodegen {

llvm::Value *builder_assign(Context &ctx, AST::ExprHandle destination, AST::ExprHandle value) {

  llvm::Value *desti_val = destination->codegen(ctx, AST::Expr::Value::L);
  llvm::Value *value_val = value->codegen(ctx, AST::Expr::Value::R);

  ctx.builder.CreateStore(value_val, desti_val, "op.assign");

  return value_val;
}

// Codegen for ptr + int
llvm::Value *builder_ptr_add(Context &ctx, AST::ExprHandle ptr, AST::ExprHandle offset) {

  auto offset_val = offset->codegen(ctx, AST::Expr::Value::R);

  auto val = ctx.builder.CreateInBoundsGEP(ptr->type()->deref()->codegen(ctx),
                                           ptr->codegen(ctx, AST::Expr::Value::R),
                                           llvm::ArrayRef<llvm::Value *>{offset_val},
                                           "add.ptr");

  return val;
}

llvm::Value *builder_add(Context &ctx, const AST::Expr::Prim2 *expr, AST::Expr::Value value) {

  llvm::Value *val;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto lhs_val = expr->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = expr->rhs->codegen(ctx, AST::Expr::Value::R);

    val = ctx.builder.CreateAdd(lhs_val, rhs_val, "op.add");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      val = builder_ptr_add(ctx, expr->lhs, expr->rhs);
    } else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      val = builder_ptr_add(ctx, expr->rhs, expr->lhs);
    } else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }

  } break;
  }

  return val;
}

// Codegen for ptr - int
llvm::Value *builder_ptr_sub(Context &ctx, AST::ExprHandle ptr, AST::ExprHandle value) {

  auto val = value->codegen(ctx, AST::Expr::Value::R);

  auto ptr_typ = ptr->type()->codegen(ctx);
  auto ptr_elem = ctx.builder.CreateNeg(val);
  auto ptr_sub = ctx.builder.CreateGEP(ptr_typ,
                                       ptr->codegen(ctx, AST::Expr::Value::R),
                                       llvm::ArrayRef<llvm::Value *>(ptr_elem),
                                       "sub");

  return ptr_sub;
}

llvm::Value *builder_sub(Context &ctx, const AST::Expr::Prim2 *expr, AST::Expr::Value value) {

  llvm::Value *val;

  switch (expr->type_kind()) {

  case AST::Typ::Kind::Bool:
  case AST::Typ::Kind::Char:
  case AST::Typ::Kind::Void: {
    throw std::logic_error("Unsupported op");
  } break;

  case AST::Typ::Kind::Int: {

    auto lhs_val = expr->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = expr->rhs->codegen(ctx, AST::Expr::Value::R);

    val = ctx.builder.CreateSub(lhs_val, rhs_val, "op.sub");

  } break;

  case AST::Typ::Kind::Ptr: {

    auto lhs_typ = expr->lhs->type();
    auto rhs_typ = expr->rhs->type();

    if (lhs_typ->is_kind(AST::Typ::Kind::Ptr) && rhs_typ->is_kind(AST::Typ::Kind::Int)) {
      val = builder_ptr_sub(ctx, expr->lhs, expr->rhs);
    } else if (lhs_typ->is_kind(AST::Typ::Kind::Int) && rhs_typ->is_kind(AST::Typ::Kind::Ptr)) {
      val = builder_ptr_sub(ctx, expr->rhs, expr->lhs);
    } else {
      throw std::logic_error("Incompatible targets for pointer arithmetic");
    }

  }

  break;
  }

  return val;
}

llvm::Value *builder_mul(Context &ctx, const AST::Expr::Prim2 *expr, AST::Expr::Value value) {
  auto lhs_val = expr->lhs->codegen(ctx, AST::Expr::Value::R);
  auto rhs_val = expr->rhs->codegen(ctx, AST::Expr::Value::R);

  return ctx.builder.CreateMul(lhs_val, rhs_val, "op.mul");
}

llvm::Value *builder_div(Context &ctx, const AST::Expr::Prim2 *expr, AST::Expr::Value value) {
  auto lhs_val = expr->lhs->codegen(ctx, AST::Expr::Value::R);
  auto rhs_val = expr->rhs->codegen(ctx, AST::Expr::Value::R);

  return ctx.builder.CreateSDiv(lhs_val, rhs_val, "op.div");
}

llvm::Value *builder_mod(Context &ctx, const AST::Expr::Prim2 *expr, AST::Expr::Value value) {
  auto lhs_val = expr->lhs->codegen(ctx, AST::Expr::Value::R);
  auto rhs_val = expr->rhs->codegen(ctx, AST::Expr::Value::R);

  return ctx.builder.CreateSRem(lhs_val, rhs_val, "op.mod");
}

} // namespace OpBinaryCodegen

llvm::Value *AST::Expr::Prim2::codegen(Context &ctx, AST::Expr::Value value) const {

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
    return OpBinaryCodegen::builder_add(ctx, this, value);
  } break;

  case OpBinary::Sub: {
    return OpBinaryCodegen::builder_sub(ctx, this, value);
  } break;

  case OpBinary::Mul: {
    return OpBinaryCodegen::builder_mul(ctx, this, value);
  } break;

  case OpBinary::Div: {
    return OpBinaryCodegen::builder_div(ctx, this, value);
  } break;

  case OpBinary::Mod: {
    return OpBinaryCodegen::builder_mod(ctx, this, value);
  } break;

  // TODO: Tidy up ops to use pointers when relevant.
  case OpBinary::Eq: {
    llvm::Value *lhs_val;
    llvm::Value *rhs_val;

    lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
    rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);

    // lhs_val->getType()->print(outs());
    // rhs_val->getType()->print(outs());

    fflush(stdout);

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_EQ, lhs_val, rhs_val, "op.eq");
  } break;

  case OpBinary::Neq: {
    llvm::Value *lhs_val;
    llvm::Value *rhs_val;

    if (lhs->typ_has_kind(AST::Typ::Kind::Ptr) && rhs->typ_has_kind(AST::Typ::Kind::Ptr)) {
      lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::L);
      rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::L);
    } else {
      lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
      rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);
    }

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_NE, lhs_val, rhs_val, "op.neq");
  } break;

  case OpBinary::Gt: {
    auto lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SGT, lhs_val, rhs_val, "op.gt");
  } break;

  case OpBinary::Lt: {
    auto lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SLT, lhs_val, rhs_val, "op.lt");
  } break;

  case OpBinary::Leq: {
    auto lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);

    return ctx.builder.CreateCmp(llvm::ICmpInst::ICMP_SLE, lhs_val, rhs_val, "op.leq");
  } break;

  case OpBinary::Geq: {
    auto lhs_val = this->lhs->codegen(ctx, AST::Expr::Value::R);
    auto rhs_val = this->rhs->codegen(ctx, AST::Expr::Value::R);

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

llvm::Value *AST::Expr::Index::codegen(Context &ctx, AST::Expr::Value value) const {

  llvm::Value *val;

  auto as_ptr = std::static_pointer_cast<AST::Typ::Ptr>(this->target->type());
  if (as_ptr->area().has_value()) {

    auto index_val = this->index->codegen(ctx, AST::Expr::Value::R);

    std::vector<llvm::Value *> array_ref = {};
    if (as_ptr->area().has_value()) {
      array_ref.push_back(ctx.get_zero());
    }
    array_ref.push_back(index_val);

    val = ctx.builder.CreateInBoundsGEP(this->target->type()->codegen(ctx),
                                        this->target->codegen(ctx, AST::Expr::Value::L),
                                        llvm::ArrayRef<llvm::Value *>(array_ref),
                                        "idx");

  } else {
    val = OpBinaryCodegen::builder_ptr_add(ctx, this->target, this->index);
  }

  if (value == AST::Expr::Value::R) {
    val = ctx.builder.CreateLoad(this->type()->codegen(ctx),
                                 val,
                                 "op.idx");
  }

  return val;
}
