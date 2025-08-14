#pragma once

#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Types.hpp"
#include "codegen/Structs.hpp"

namespace AST {

namespace Expr {

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> arguments;

  Call(TypHandle return_typ, std::string name, std::vector<ExprHandle> args)
      : name(name),
        arguments(args) {
    this->_typ = return_typ;
  }

  Expr::Kind kind() const override {
    return Expr::Kind::Call;
  }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

// CstI

struct Cast : ExprT {
  ExprHandle expr;

  Cast(ExprHandle expr, TypHandle to)
      : expr(expr) {
    this->_typ = to;
  }

  Expr::Kind kind() const override { return Expr::Kind::Cast; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

// CstI

struct CstI : ExprT {
  int64_t i;

  CstI(TypHandle typ, int64_t i)
      : i(i) {
    this->_typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::CstI; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Index

struct Index : ExprT {
  ExprHandle target;
  ExprHandle index;

  Index(ExprHandle expr, ExprHandle index)
      : target(expr),
        index(index) {
    this->_typ = this->target->typ()->deref();
  }

  Expr::Kind kind() const override { return Expr::Kind::Index; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Prim1

struct Prim1 : ExprT {
  OpUnary op;
  ExprHandle expr;

  Prim1(TypHandle typ, OpUnary op, ExprHandle expr)
      : op(op),
        expr(expr) {
    this->_typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Prim2

struct Prim2 : ExprT {
  AST::Expr::OpBinary op;
  ExprHandle lhs;
  ExprHandle rhs;

  Prim2(TypHandle typ, AST::Expr::OpBinary op, ExprHandle lhs, ExprHandle rhs)
      : op(op),
        lhs(lhs),
        rhs(rhs) {
    this->_typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

struct Var : ExprT {
  std::string var;

  Var(TypHandle typ, std::string var) : var(var) {
    this->_typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Var; }

  llvm::Value *codegen(Context &ctx, AST::Expr::Value value) const override;
  std::string to_string(size_t indent = 0) const override;
};

} // namespace Expr

//
} // namespace AST
