#pragma once

#include <cstdint>
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

  Call(TypHandle return_type, std::string name, std::vector<ExprHandle> params)
      : name(name),
        arguments(std::move(params)) {
    this->typ = return_type;
  }

  Expr::Kind kind() const override {
    return Expr::Kind::Call;
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

// CstI

struct CstI : ExprT {
  int64_t i;

  CstI(TypHandle type, int64_t i) : i(i) {
    this->typ = type;
  }

  Expr::Kind kind() const override { return Expr::Kind::CstI; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Index

struct Index : ExprT {
  ExprHandle access;
  ExprHandle index;

  Index(ExprHandle expr, ExprHandle index) : access(expr),
                                             index(index) {
    // Dereference the expression being accessed
    this->typ = this->access->type()->deref();
  }

  Expr::Kind kind() const override { return Expr::Kind::Index; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Prim1

struct Prim1 : ExprT {
  OpUnary op;
  ExprHandle expr;

  Prim1(TypHandle typ, OpUnary op, ExprHandle expr)
      : op(op),
        expr(std::move(expr)) {
    this->typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Prim2

struct Prim2 : ExprT {
  AST::Expr::OpBinary op;
  ExprHandle lhs;
  ExprHandle rhs;

  Prim2(TypHandle typ, AST::Expr::OpBinary op, ExprHandle lhs, ExprHandle rhs)
      : op(op),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)) {
    this->typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

struct Var : ExprT {
  std::string var;

  Var(TypHandle typ, std::string &&v) : var(std::move(v)) {
    this->typ = typ;
  }

  Expr::Kind kind() const override { return Expr::Kind::Var; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent = 0) const override;
};

} // namespace Expr

//
} // namespace AST
