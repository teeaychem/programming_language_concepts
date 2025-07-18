#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "AST/AST.hh"
#include "AST/Block.hh"

namespace AST {
namespace Stmt {

// Block

struct Block : StmtT {
  AST::Block block{};

  Block(AST::Block &&bv)
      : block(std::move(bv)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Block; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
  [[nodiscard]] bool returns() const override { return this->block.returns; };
};

// Expr

struct Expr : StmtT {
  ExprHandle expr;

  Expr(ExprHandle expr)
      : expr(std::move(expr)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Expr; }

  llvm::Value *codegen(LLVMBundle &hdl) override;
  [[nodiscard]] bool returns() const override { return false; };
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
  llvm::Value *codegen(LLVMBundle &hdl) override;
  [[nodiscard]] bool returns() const override { return this->stmt_then->block.returns && this->stmt_else->block.returns; };
};

// Return

struct Return : StmtT {
  std::optional<ExprHandle> value;

  Return(std::optional<ExprHandle> value)
      : value(std::move(value)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Return; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
  [[nodiscard]] bool returns() const override { return true; };
};

// While

struct While : StmtT {
  ExprHandle condition;
  StmtHandle stmt;

  While(ExprHandle condition, StmtHandle bv)
      : condition(condition), stmt(bv) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
  [[nodiscard]] bool returns() const override {
    std::cout << "TODO: While returns?" << std::endl;
    exit(-1);

    return false;
  };
};

} // namespace Stmt
} // namespace AST
