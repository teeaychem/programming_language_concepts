#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"

#include "codegen/Structs.hpp"

std::pair<llvm::Value *, AST::TypHandle> Context::access(AST::ExprT const *expr) {

  auto value = expr->codegen(*this);
  auto type = access_type(expr);

  switch (expr->kind()) {

    // Only Index, Prim1, and Var require additional action.
  case AST::Expr::Kind::Call:
  case AST::Expr::Kind::Cast:
  case AST::Expr::Kind::CstI:
  case AST::Expr::Kind::Prim2: {
  } break;

  case AST::Expr::Kind::Index: {
    value = this->builder.CreateLoad(type->codegen(*this), value, "acc.idx");
  } break;

    // Only dereference requires additional action
  case AST::Expr::Kind::Prim1: {

    switch (((AST::Expr::Prim1 *)(expr))->op) {

    case AST::Expr::OpUnary::AddressOf:
    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
    } break;

    case AST::Expr::OpUnary::Dereference: {
      value = this->builder.CreateLoad(type->codegen(*this), value, "acc.drf");
    } break;
    }

  } break;

  case AST::Expr::Kind::Var: {
    auto as_var = (AST::Expr::Var *)expr;

    switch (expr->type_kind()) {

    case AST::Typ::Kind::Bool:
    case AST::Typ::Kind::Char:
    case AST::Typ::Kind::Int: {
      value = this->builder.CreateLoad(type->codegen(*this), value, as_var->var);
    } break;

    case AST::Typ::Kind::Ptr: {
      auto ptr_typ = std::static_pointer_cast<AST::Typ::Ptr>(type);

      // Decay a pointer to an array
      if (ptr_typ->area().has_value()) {
        value = this->builder.CreateInBoundsGEP(expr->type()->codegen(*this), value, llvm::ArrayRef(this->get_zero()), "acc.dcy");
      }
      // Otherwise, load
      else {
        value = this->builder.CreateLoad(expr->type()->codegen(*this), value, "acc.ptr");
      }
    } break;

    case AST::Typ::Kind::Void: {
      throw std::logic_error("Access to void");
    } break;
    }

  } break;
  }

  return {value, type};
}

// For most expressions the type accessed is the type associated with the expression.
// E.g., accessing var of int type returns something of int type.
// The exceptions are pointer deref through indexing or direct deref.
// Here, the type is obtained through dereference.
AST::TypHandle Context::access_type(AST::ExprT const *expr) {

  auto typ = expr->type();

  switch (expr->kind()) {

  case AST::Expr::Kind::Call:
  case AST::Expr::Kind::Cast:
  case AST::Expr::Kind::CstI:
  case AST::Expr::Kind::Prim2:
  case AST::Expr::Kind::Var: {
  } break;

  case AST::Expr::Kind::Index: {
    auto as_index = (AST::Expr::Index *)(expr);
    typ = as_index->target->type()->deref();
  } break;

  case AST::Expr::Kind::Prim1: {
    auto as_prim1 = (AST::Expr::Prim1 *)(expr);

    switch (as_prim1->op) {
    case AST::Expr::OpUnary::AddressOf:
    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
    } break;

    case AST::Expr::OpUnary::Dereference: {
      typ = as_prim1->expr->type()->deref();
    } break;
    }
  } break;
  }

  return typ;
}
