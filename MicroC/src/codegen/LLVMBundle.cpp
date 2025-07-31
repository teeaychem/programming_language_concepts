#include "LLVMBundle.hpp"
#include "AST/AST.hpp"
#include "AST/Node/Expr.hpp"

llvm::Value *LLVMBundle::access_if(AST::ExprHandle expr, llvm::Value *value) {

  if (expr == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  if (value == nullptr) {
    throw std::logic_error("Unable to load without type information");
  }

  if (expr->kind() == AST::Expr::Kind::Var) {
    return this->builder.CreateLoad(expr->type()->llvm(*this), value);
  }

  else if (expr->kind() == AST::Expr::Kind::Index) {
    return this->builder.CreateLoad(expr->type()->llvm(*this), value);
  }

  else if (expr->kind() == AST::Expr::Kind::Prim1) {
    auto as_prim1 = std::static_pointer_cast<AST::Expr::Prim1>(expr);

    switch (as_prim1->op) {
    case AST::Expr::OpUnary::AddressOf:
    case AST::Expr::OpUnary::Dereference: {
      return this->builder.CreateLoad(expr->type()->llvm(*this), value);
    } break;

    case AST::Expr::OpUnary::Sub:
    case AST::Expr::OpUnary::Negation: {
    } break;
    }
  }

  return value;
}
