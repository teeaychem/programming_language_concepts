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

// Assign

struct Assign : ExprT {
  // TODO: Checks on destination
  ExprHandle dest;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Assign; }
  std::string to_string(size_t indent) const override;
  TypHandle type() const override {
    std::cout << "Asssign::type() - " << dest->type()->to_string(0) << std::endl;
    return this->dest->type();
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Assign(ExprHandle dest, ExprHandle expr) : dest(dest), expr(expr) {}
};

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> arguments;
  TypHandle r_typ;

  Expr::Kind kind() const override { return Expr::Kind::Call; }
  std::string to_string(size_t indent) const override;
  TypHandle type() const override { return r_typ; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Call(std::string name, TypHandle r_typ, std::vector<ExprHandle> params)
      : name(name), r_typ(r_typ), arguments(std::move(params)) {
  }
};

// CstI

struct CstI : ExprT {
  int64_t i;

  Expr::Kind kind() const override { return Expr::Kind::CstI; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
  TypHandle type() const override { return Typ::pk_Data(Typ::Data::Int); }

  CstI(int64_t i) : i(i) {}
};

// Index

struct Index : ExprT {
  ExprHandle access;
  ExprHandle index;

  Expr::Kind kind() const override { return Expr::Kind::Index; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;

  // TODO: Verify correct type
  TypHandle type() const override { return this->access->type()->deref_unsafe(); }

  Index(ExprHandle expr, ExprHandle index) : access(expr), index(index) {}
};

// Prim1

struct Prim1 : ExprT {
  std::string op;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }
  std::string to_string(size_t indent) const override;

  TypHandle type() const override { return this->expr->type(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Prim1(std::string op, ExprHandle expr)
      : op(op), expr(std::move(expr)) {}
};

// Prim2

struct Prim2 : ExprT {
  std::string op;
  ExprHandle a;
  ExprHandle b;

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }
  std::string to_string(size_t indent) const override;

  TypHandle type() const override { return this->a->type(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Prim2(std::string op, ExprHandle a, ExprHandle b)
      : op(op), a(std::move(a)), b(std::move(b)) {}
};

struct Var : ExprT {
  std::string var;
  TypHandle typ;

  Var(TypHandle typ, std::string &&v) : typ(typ), var(std::move(v)) {}

  std::string to_string(size_t indent) const override;
  Expr::Kind kind() const override { return Expr::Kind::Var; }
  TypHandle type() const override { return this->typ; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

} // namespace Expr

//
} // namespace AST
