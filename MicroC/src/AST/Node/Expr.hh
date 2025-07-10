#pragma once

#include "AST/AST.hh"
#include <cstdint>
#include <fmt/base.h>
#include <fmt/format.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"

namespace AST {

namespace Expr {

// Access

struct Access : ExprT {
  enum class Mode {
    Access,
    Addr,
  };

  Mode mode;
  AccessHandle acc;

  Expr::Kind kind() const override { return Expr::Kind::Access; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Access(Access::Mode mode, AccessHandle acc) : mode(mode), acc(std::move(acc)) {}
};

inline ExprHandle pk_Access(Access::Mode mode, AccessHandle acc) {
  Access e(mode, std::move(acc));
  return std::make_shared<Access>(std::move(e));
}

// Assign

struct Assign : ExprT {

  AccessHandle dest;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Access; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Assign(AccessHandle dest, ExprHandle expr) : dest(dest), expr(expr) {}
};

inline ExprHandle pk_Assign(AccessHandle dest, ExprHandle expr) {
  Assign e(dest, expr);
  return std::make_shared<Assign>(std::move(e));
}

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> parameters;

  Expr::Kind kind() const override { return Expr::Kind::Call; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Call(std::string name, std::vector<ExprHandle> params)
      : name(name), parameters(std::move(params)) {}
};

inline ExprHandle pk_Call(std::string name, std::vector<ExprHandle> params) {
  Call e(std::move(name), std::move(params));
  return std::make_shared<Call>(std::move(e));
}

// CstI

struct CstI : ExprT {
  int64_t i;

  Expr::Kind kind() const override { return Expr::Kind::CstI; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  CstI(int64_t i) : i(i) {}
};

inline ExprHandle pk_CstI(std::int64_t i) {
  CstI e(i);
  return std::make_shared<CstI>(std::move(e));
}

// Prim1

struct Prim1 : ExprT {
  std::string op;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Prim1(std::string op, ExprHandle expr)
      : op(op), expr(std::move(expr)) {}
};

inline ExprHandle pk_Prim1(std::string op, ExprHandle expr) {
  Prim1 e(op, std::move(expr));
  return std::make_shared<Prim1>(std::move(e));
}

// Prim2

struct Prim2 : ExprT {
  std::string op;
  ExprHandle a;
  ExprHandle b;

  Expr::Kind kind() const override { return Expr::Kind::Prim2; }
  std::string to_string() const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Prim2(std::string op, ExprHandle a, ExprHandle b)
      : op(op), a(std::move(a)), b(std::move(b)) {}
};

inline ExprHandle pk_Prim2(std::string op, ExprHandle a, ExprHandle b) {
  Prim2 e(op, std::move(a), std::move(b));
  return std::make_shared<Prim2>(std::move(e));
}

} // namespace Expr

//
} // namespace AST
