#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "AST/AST.hh"

namespace AST {
namespace Stmt {

// Block

struct Block : StmtT {
  BlockVec block{};

  Block(BlockVec &&bv)
      : block(std::move(bv)) {}

  std::string to_string() const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Block; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline StmtHandle pk_Block(BlockVec &&bv) {
  Block b(std::move(bv));
  return std::make_shared<Block>(std::move(b));
}

// Expr

struct Expr : StmtT {
  ExprHandle expr;

  Expr(ExprHandle expr)
      : expr(std::move(expr)) {}

  std::string to_string() const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Expr; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline StmtHandle pk_Expr(ExprHandle expr) {
  Expr e(std::move(expr));
  return std::make_shared<Expr>(std::move(e));
}

// If

struct If : StmtT {
  ExprHandle condition;
  StmtHandle yes;
  StmtHandle no;

  If(ExprHandle condition, StmtHandle yes, StmtHandle no)
      : condition(condition), yes(yes), no(no) {}

  std::string to_string() const override;
  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline StmtHandle pk_If(ExprHandle condition, StmtHandle yes, StmtHandle no) {
  If e(std::move(condition), std::move(yes), std::move(no));
  return std::make_shared<If>(std::move(e));
}

// Return

struct Return : StmtT {
  std::optional<ExprHandle> value;

  Return(std::optional<ExprHandle> value)
      : value(std::move(value)) {}

  std::string to_string() const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Return; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline StmtHandle pk_Return(std::optional<ExprHandle> value) {
  Return e(std::move(value));
  return std::make_shared<Return>(std::move(e));
}

// While

struct While : StmtT {
  ExprHandle condition;
  StmtHandle block;

  While(ExprHandle condition, StmtHandle bv)
      : condition(condition), block(bv) {}

  std::string to_string() const override;
  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline StmtHandle pk_While(ExprHandle condition, StmtHandle bv) {
  While e(condition, bv);
  return std::make_shared<While>(std::move(e));
}

} // namespace Stmt
} // namespace AST
