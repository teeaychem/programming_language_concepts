#pragma once

#include <optional>
#include <string>

#include "AST/AST.hpp"
#include "AST/Block.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {
namespace Stmt {

// Block

struct Block : StmtT {
  AST::Block block{};

  Block(AST::Block &&bv)
      : block(std::move(bv)) {}

  Stmt::Kind kind() const override { return Stmt::Kind::Block; }
  bool returns() const override { return this->block.returns; };
  size_t early_returns() const override { return this->block.early_returns; };
  size_t pass_throughs() const override { return this->block.pass_throughs; };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// Declaration

struct Declaration : StmtT {
  DecHandle declaration;

  Declaration(DecHandle expr)
      : declaration(std::move(expr)) {}

  Stmt::Kind kind() const override { return Stmt::Kind::Declaration; }
  bool returns() const override { return false; };
  size_t early_returns() const override { return 0; };
  size_t pass_throughs() const override { return 0; };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// Expr

struct Expr : StmtT {
  ExprHandle expr;

  Expr(ExprHandle expr)
      : expr(std::move(expr)) {}

  Stmt::Kind kind() const override { return Stmt::Kind::Expr; }
  bool returns() const override { return false; };
  size_t early_returns() const override { return 0; };
  size_t pass_throughs() const override { return 0; };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// If

struct If : StmtT {
  ExprHandle condition;
  StmtBlockHandle stmt_then;
  StmtBlockHandle stmt_else;

  If(ExprHandle condition, StmtBlockHandle stmt_then, StmtBlockHandle stmt_else)
      : condition(condition),
        stmt_then(stmt_then),
        stmt_else(stmt_else) {}

  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  bool returns() const override { return this->stmt_then->block.returns && this->stmt_else->block.returns; };
  size_t early_returns() const override { return this->stmt_then->early_returns() + this->stmt_else->early_returns(); };
  size_t pass_throughs() const override { return this->stmt_then->pass_throughs() + this->stmt_else->pass_throughs(); };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// Return

struct Return : StmtT {
  std::optional<ExprHandle> value;

  Return(std::optional<ExprHandle> value)
      : value(std::move(value)) {}

  Stmt::Kind kind() const override { return Stmt::Kind::Return; }
  bool returns() const override { return true; };
  size_t early_returns() const override { return 0; };
  size_t pass_throughs() const override { return 0; };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// While

struct While : StmtT {
  ExprHandle condition;
  StmtHandle body;

  While(ExprHandle condition, StmtHandle body)
      : condition(condition),
        body(body) {}

  Stmt::Kind kind() const override { return Stmt::Kind::While; }
  bool returns() const override { return false; };
  size_t early_returns() const override { return this->body->early_returns(); };
  size_t pass_throughs() const override { return this->body->pass_throughs(); };

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

} // namespace Stmt
} // namespace AST
