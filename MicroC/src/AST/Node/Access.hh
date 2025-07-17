#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "AST/AST.hh"
#include "AST/Types.hh"

namespace AST {
namespace Access {

// Var

struct Var : AccessT {
  std::string var;
  TypHandle typ;

  Var(TypHandle typ, std::string &&v) : typ(typ), var(std::move(v)) {}

  std::string to_string(size_t indent) const override;
  Access::Kind kind() const override { return Access::Kind::Var; }
  TypHandle eval_type() const override { return typ; }

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Deref

struct Deref : AccessT {
  ExprHandle expr;

  Deref(ExprHandle &&expr) : expr(std::move(expr)) {}

  Access::Kind kind() const override { return Access::Kind::Deref; }
  std::string to_string(size_t indent) const override;
  TypHandle eval_type() const override { return expr->type()->deref_unsafe(); }

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Index

struct Index : AccessT {
  AccessHandle access;
  ExprHandle index;

  Index(AccessHandle &&arr, ExprHandle &&idx)
      : access(std::move(arr)), index(std::move(idx)) {}

  Access::Kind kind() const override { return Access::Kind::Index; }
  std::string to_string(size_t indent) const override;
  TypHandle eval_type() const override { return access->eval_type()->deref_unsafe(); }

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

} // namespace Access
} // namespace AST
