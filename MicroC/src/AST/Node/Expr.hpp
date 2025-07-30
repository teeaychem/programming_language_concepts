#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Types.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {

namespace Expr {

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> arguments;

  Call(std::string name, std::vector<ExprHandle> params, TypHandle r_typ)
      : name(name),
        arguments(std::move(params)) {
    this->typ = r_typ;
  }

  Expr::Kind kind() const override {
    return Expr::Kind::Call;
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// CstI

struct CstI : ExprT {
  int64_t i;

  CstI(int64_t i, TypHandle typ) : i(i) {
    this->typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::CstI; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Index

struct Index : ExprT {
  ExprHandle access;
  ExprHandle index;

  Index(ExprHandle expr, ExprHandle index) : access(expr),
                                             index(index) {
    // FIXME: Correct type
    this->typ = this->access->type()->deref_unsafe();
  }

  Expr::Kind kind() const override { return Expr::Kind::Index; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Prim1

struct Prim1 : ExprT {
  std::string op;
  ExprHandle expr;

  Prim1(std::string op, ExprHandle expr)
      : op(op),
        expr(std::move(expr)) {
    // FIXME: Fix type
    this->typ = this->expr->type();
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Prim2

struct Prim2 : ExprT {
  std::string op;
  ExprHandle a;
  ExprHandle b;

  Prim2(std::string op, ExprHandle a, ExprHandle b)
      : op(op), a(std::move(a)), b(std::move(b)) {
    // FIXME: Fix type
    this->typ = this->a->type();
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

struct Var : ExprT {
  std::string var;

  Var(TypHandle typ, std::string &&v) : var(std::move(v)) {
    this->typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Var; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

} // namespace Expr

//
} // namespace AST
