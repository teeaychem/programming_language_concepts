#pragma once

#include "AST/AST.hh"
#include <cstdint>
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
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Access(Access::Mode mode, AccessHandle acc) : mode(mode), acc(std::move(acc)) {}
};

// Assign

struct Assign : ExprT {

  AccessHandle dest;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Access; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Assign(AccessHandle dest, ExprHandle expr) : dest(dest), expr(expr) {}
};

// Call

struct Call : ExprT {
  std::string name;
  std::vector<ExprHandle> parameters;

  Expr::Kind kind() const override { return Expr::Kind::Call; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Call(std::string name, std::vector<ExprHandle> params)
      : name(name), parameters(std::move(params)) {}
};

// CstI

struct CstI : ExprT {
  int64_t i;

  Expr::Kind kind() const override { return Expr::Kind::CstI; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

  CstI(int64_t i) : i(i) {}
};

// Prim1

struct Prim1 : ExprT {
  std::string op;
  ExprHandle expr;

  Expr::Kind kind() const override { return Expr::Kind::Prim1; }
  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;

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
  llvm::Value *codegen(LLVMBundle &hdl) override;

  Prim2(std::string op, ExprHandle a, ExprHandle b)
      : op(op), a(std::move(a)), b(std::move(b)) {}
};

} // namespace Expr

//
} // namespace AST
