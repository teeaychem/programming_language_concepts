#include "LLVMBundle.hpp"
#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"

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
    auto llvm = this->builder.CreateLoad(type->llvm(*this), value);

    return {llvm, type};
  } break;

  case AST::Expr::Kind::Prim1: {
    auto as_prim1 = (AST::Expr::Prim1 *)(expr);

    switch (as_prim1->op) {
    case AST::Expr::OpUnary::AddressOf: {
      return {value, expr->type()};
    } break;

    case AST::Expr::OpUnary::Dereference: {

      auto type = as_prim1->expr->type()->deref();
      auto llvm = this->builder.CreateLoad(type->llvm(*this), value);

      return {llvm, type};
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
    auto type = expr->type();
    auto llvm = this->builder.CreateLoad(type->llvm(*this), value);

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
