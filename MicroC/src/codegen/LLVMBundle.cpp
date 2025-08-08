#include "LLVMBundle.hpp"
#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"
#include <memory>

std::pair<llvm::Value *, AST::TypHandle> LLVMBundle::access(AST::ExprT const *expr) {

  auto value = expr->codegen(*this);

  if (expr == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  if (value == nullptr) {
    throw std::logic_error("Unable to load without value information");
  }

  switch (expr->kind()) {

  case AST::Expr::Kind::Call: {
    return {value, expr->type()};
  } break;

  case AST::Expr::Kind::Cast: {
    return {value, expr->type()};
  } break;

  case AST::Expr::Kind::CstI: {
    return {value, expr->type()};
  } break;

  case AST::Expr::Kind::Index: {
    auto as_index = (AST::Expr::Index *)(expr);

    auto type = as_index->target->type()->deref();
    auto llvm = this->builder.CreateLoad(type->codegen(*this), value, "acc.idx");

    return {llvm, type};
  } break;

  case AST::Expr::Kind::Prim1: {
    auto as_prim1 = (AST::Expr::Prim1 *)(expr);

    switch (as_prim1->op) {

    case AST::Expr::OpUnary::AddressOf: {
      // The access action in this case is *withholding* of a load.
      // And, so, returning the address of the expression.
      return {value, expr->type()};
    } break;

    case AST::Expr::OpUnary::Dereference: {

      auto type = as_prim1->type();
      value = this->builder.CreateLoad(type->codegen(*this), value, "acc.drf");

      return {value, type};
    } break;

    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
      return {value, expr->type()};
    } break;
    }
  } break;

  case AST::Expr::Kind::Prim2: {
    return {value, expr->type()};
  } break;

  case AST::Expr::Kind::Var: {

    auto as_var = (AST::Expr::Var *)expr;

    switch (expr->type_kind()) {

    case AST::Typ::Kind::Bool:
    case AST::Typ::Kind::Char:
    case AST::Typ::Kind::Int: {
      auto type = expr->type();
      auto llvm = this->builder.CreateLoad(type->codegen(*this), value, as_var->var);

      return {llvm, type};
    } break;

    case AST::Typ::Kind::Ptr: {

      auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(expr->type());

      auto type = std::static_pointer_cast<AST::Typ::Ptr>(expr->type());
      // auto llvm = this->builder.CreateLoad(type->llvm(*this), value, "acc.ptr");

      if (ptr_typ->area().has_value()) {
        llvm::Value *MC_INT = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*this->context), 0);
        value = this->builder.CreateInBoundsGEP(expr->type()->codegen(*this), value, llvm::ArrayRef(MC_INT), "acc.ptr.decay");
      } else {
        value = this->builder.CreateLoad(expr->type()->codegen(*this), value, "acc.ptr");
      }

      return {value, type};

    } break;

    case AST::Typ::Kind::Void: {
      throw std::logic_error("Access to void");
    } break;
    }

    auto type = expr->type();
    auto llvm = this->builder.CreateLoad(type->codegen(*this), value, "acc.vd");

    return {llvm, type};
  } break;
  }
}

AST::TypHandle LLVMBundle::access_type(AST::ExprT const *expr) {

  if (expr == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  switch (expr->kind()) {

  case AST::Expr::Kind::Call: {
    return expr->type();
  } break;

  case AST::Expr::Kind::Cast: {
    return expr->type();
  } break;

  case AST::Expr::Kind::CstI: {
    return expr->type();
  } break;

  case AST::Expr::Kind::Index: {
    auto as_index = (AST::Expr::Index *)(expr);
    return as_index->target->type()->deref();
  } break;

  case AST::Expr::Kind::Prim1: {
    auto as_prim1 = (AST::Expr::Prim1 *)(expr);

    switch (as_prim1->op) {
    case AST::Expr::OpUnary::AddressOf: {
      return expr->type();
    } break;

    case AST::Expr::OpUnary::Dereference: {
      return as_prim1->expr->type()->deref();
    } break;

    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
      return expr->type();
    } break;
    }
  } break;

  case AST::Expr::Kind::Prim2: {
    return expr->type();
  } break;

  case AST::Expr::Kind::Var: {
    return expr->type();
  } break;
  }
}
