#pragma once

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
};

// Expr

struct Expr : StmtT {
  ExprHandle expr;

  Expr(ExprHandle expr)
      : expr(std::move(expr)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Expr; }

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// If

struct If : StmtT {
  ExprHandle condition;
  StmtBlockHandle thn;
  StmtBlockHandle els;

  If(ExprHandle condition, StmtBlockHandle yes, StmtBlockHandle no)
      : condition(condition), thn(yes), els(no) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::If; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Return

struct Return : StmtT {
  std::optional<ExprHandle> value;

  Return(std::optional<ExprHandle> value)
      : value(std::move(value)) {}

  std::string to_string(size_t indent) const override;
  Stmt::Kind kind() const override { return Stmt::Kind::Return; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
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
};

} // namespace Stmt
} // namespace AST
