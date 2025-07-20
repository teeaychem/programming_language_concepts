#pragma once

#include <optional>
#include <string>

#include "AST/AST.hpp"
#include "AST/Block.hpp"

namespace AST {
namespace Stmt {

// Block

struct Block : StmtT {
  AST::Block block{};

  Block(AST::Block &&bv)
      : block(std::move(bv)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Block; }
  llvm::Value *codegen(LLVMBundle &hdl) const override;
  [[nodiscard]] bool returns() const override { return this->block.returns; };
  [[nodiscard]] size_t early_returns() const override { return this->block.early_returns; };
  [[nodiscard]] size_t pass_throughs() const override { return this->block.pass_throughs; };
};

// Expr

struct Expr : StmtT {
  ExprHandle expr;

  Expr(ExprHandle expr)
      : expr(std::move(expr)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Expr; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  [[nodiscard]] bool returns() const override { return false; };
  [[nodiscard]] size_t early_returns() const override { return 0; };
  [[nodiscard]] size_t pass_throughs() const override { return 0; };
};

// If

struct If : StmtT {
  ExprHandle condition;
  StmtBlockHandle stmt_then;
  StmtBlockHandle stmt_else;

  If(ExprHandle condition, StmtBlockHandle stmt_then, StmtBlockHandle stmt_else)
      : condition(condition), stmt_then(stmt_then), stmt_else(stmt_else) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  llvm::Value *codegen(LLVMBundle &hdl) const override;
  [[nodiscard]] bool returns() const override { return this->stmt_then->block.returns && this->stmt_else->block.returns; };
  [[nodiscard]] size_t early_returns() const override { return this->stmt_then->early_returns() + this->stmt_else->early_returns(); };
  [[nodiscard]] size_t pass_throughs() const override { return this->stmt_then->pass_throughs() + this->stmt_else->pass_throughs(); };
};

// Return

struct Return : StmtT {
  std::optional<ExprHandle> value;

  Return(std::optional<ExprHandle> value)
      : value(std::move(value)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Return; }
  llvm::Value *codegen(LLVMBundle &hdl) const override;
  [[nodiscard]] bool returns() const override { return true; };
  [[nodiscard]] size_t early_returns() const override { return 0; };
  [[nodiscard]] size_t pass_throughs() const override { return 0; };
};

// While

struct While : StmtT {
  ExprHandle condition;
  StmtHandle stmt;

  While(ExprHandle condition, StmtHandle bv)
      : condition(condition), stmt(bv) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::While; }
  llvm::Value *codegen(LLVMBundle &hdl) const override;
  [[nodiscard]] bool returns() const override { return false; };
  [[nodiscard]] size_t early_returns() const override { return this->stmt->early_returns(); };
  [[nodiscard]] size_t pass_throughs() const override { return this->stmt->pass_throughs(); };
};

} // namespace Stmt
} // namespace AST
