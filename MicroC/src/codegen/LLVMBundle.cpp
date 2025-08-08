#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"

#include "LLVMBundle.hpp"

std::pair<llvm::Value *, AST::TypHandle> LLVMBundle::access(AST::ExprT const *expr) {

  auto value = expr->codegen(*this);
  if (value == nullptr) {
    throw std::logic_error("Unable to load without value information");
  }

  if (expr == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  auto type = access_type(expr);

  switch (expr->kind()) {

  case AST::Expr::Kind::Call:
  case AST::Expr::Kind::Cast:
  case AST::Expr::Kind::CstI: {
    return {value, type};
  } break;

  case AST::Expr::Kind::Index: {
    value = this->builder.CreateLoad(type->codegen(*this), value, "acc.idx");
    return {value, type};
  } break;

  case AST::Expr::Kind::Prim1: {

    switch (((AST::Expr::Prim1 *)(expr))->op) {

    case AST::Expr::OpUnary::AddressOf: {
      // The access action in this case is *withholding* of a load.
    } break;

    case AST::Expr::OpUnary::Dereference: {

      value = this->builder.CreateLoad(type->codegen(*this), value, "acc.drf");

    } break;

    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
    } break;
    }

    return {value, type};
  } break;

  case AST::Expr::Kind::Prim2: {
    return {value, type};
  } break;

  case AST::Expr::Kind::Var: {
    auto as_var = (AST::Expr::Var *)expr;

    switch (expr->type_kind()) {

    case AST::Typ::Kind::Bool:
    case AST::Typ::Kind::Char:
    case AST::Typ::Kind::Int: {
      value = this->builder.CreateLoad(type->codegen(*this), value, as_var->var);

      return {value, type};
    } break;

    case AST::Typ::Kind::Ptr: {
      auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(type);

      if (ptr_typ->area().has_value()) {
        value = this->builder.CreateInBoundsGEP(expr->type()->codegen(*this), value, llvm::ArrayRef(this->get_zero()), "acc.ptr.decay");
      } else {
        value = this->builder.CreateLoad(expr->type()->codegen(*this), value, "acc.ptr");
      }

      return {value, type};

    } break;

    case AST::Typ::Kind::Void: {
      throw std::logic_error("Access to void");
    } break;
    }

    value = this->builder.CreateLoad(type->codegen(*this), value, "acc.vd");

    return {value, type};
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
