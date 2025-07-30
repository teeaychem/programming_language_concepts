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

  Assign(ExprHandle dest, ExprHandle expr) : dest(dest),
                                             expr(expr) {}

  Expr::Kind kind() const override { return Expr::Kind::Assign; }
  TypHandle type() const override {
    std::cout << "Asssign::type() - " << dest->type()->to_string(0) << std::endl;
    return this->dest->type();
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

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
  TypHandle type() const override {
    return this->typ;
  }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// CstI

struct CstI : ExprT {
  int64_t i;

  CstI(int64_t i) : i(i) {}

  Expr::Kind kind() const override { return Expr::Kind::CstI; }
  TypHandle type() const override { return Typ::pk_Data(Typ::Data::Int); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Index

struct Index : ExprT {
  ExprHandle access;
  ExprHandle index;

  Index(ExprHandle expr, ExprHandle index) : access(expr),
                                             index(index) {}

  Expr::Kind kind() const override { return Expr::Kind::Index; }
  // TODO: Verify correct type
  TypHandle type() const override { return this->access->type()->deref_unsafe(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Prim1

struct Prim1 : ExprT {
  std::string op;
  ExprHandle expr;

  Prim1(std::string op, ExprHandle expr)
      : op(op),
        expr(std::move(expr)) {}

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }
  TypHandle type() const override { return this->expr->type(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Prim2

struct Prim2 : ExprT {
  std::string op;
  ExprHandle a;
  ExprHandle b;

  Prim2(std::string op, ExprHandle a, ExprHandle b)
      : op(op), a(std::move(a)), b(std::move(b)) {}

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }
  TypHandle type() const override { return this->a->type(); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

struct Var : ExprT {
  std::string var;
  TypHandle typ;

  Var(TypHandle typ, std::string &&v) : typ(typ),
                                        var(std::move(v)) {}

  Expr::Kind kind() const override { return Expr::Kind::Var; }
  TypHandle type() const override { return this->typ; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

} // namespace Expr

//
} // namespace AST
