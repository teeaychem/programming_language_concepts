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
  void type_resolution(Env &env) override;
};

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> arguments;
  TypHandle r_typ;

  Call(std::string name, TypHandle r_typ, std::vector<ExprHandle> params)
      : name(name),
        r_typ(r_typ),
        arguments(std::move(params)) {
  }

  Expr::Kind kind() const override { return Expr::Kind::Call; }
  TypHandle type() const override { return r_typ; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
  void type_resolution(Env &env) override;
};

// CstI

struct CstI : ExprT {
  int64_t i;

  CstI(int64_t i) : i(i) {}

  Expr::Kind kind() const override { return Expr::Kind::CstI; }
  TypHandle type() const override { return Typ::pk_Data(Typ::Data::Int); }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
  void type_resolution(Env &env) override;
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
  void type_resolution(Env &env) override;
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
  void type_resolution(Env &env) override;
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
  void type_resolution(Env &env) override;
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
  void type_resolution(Env &env) override;
};

} // namespace Expr

//
} // namespace AST
