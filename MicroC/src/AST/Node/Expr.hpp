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

// Access

struct Access : ExprT {
  AccessHandle acc;

  Expr::Kind kind() const override { return Expr::Kind::Access; }
  std::string to_string(size_t indent) const override;
  TypHandle type() const override { return acc->eval_type(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Access(AccessHandle acc) : acc(std::move(acc)) {}
};

// Assign

struct Assign : ExprT {
  AccessHandle dest;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Access; }
  std::string to_string(size_t indent) const override;
  TypHandle type() const override {
    std::cout << "Asssign::type() - " << dest->eval_type()->to_string(0) << std::endl;
    return this->dest->eval_type();
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;

  Assign(AccessHandle dest, ExprHandle expr) : dest(dest), expr(expr) {}
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
  AccessHandle access;
  ExprHandle index;

  Expr::Kind kind() const override { return Expr::Kind::Index; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;

  TypHandle type() const override { return this->access->eval_type()->deref_unsafe(); }

  Index(AccessHandle expr, ExprHandle index) : access(expr), index(index) {}
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

} // namespace Expr

//
} // namespace AST
