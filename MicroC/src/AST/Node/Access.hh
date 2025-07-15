#pragma once

#include <memory>
#include <string>

#include "AST/AST.hh"

namespace AST {
namespace Access {

// Var

struct Var : AccessT {
  std::string var;

  Var(std::string &&v) : var(std::move(v)) {}

  std::string to_string(size_t indent) const override;
  Access::Kind kind() const override { return Access::Kind::Var; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Deref

struct Deref : AccessT {
  ExprHandle expr;

  Deref(ExprHandle &&expr) : expr(std::move(expr)) {}

  Access::Kind kind() const override { return Access::Kind::Deref; }
  std::string to_string(size_t indent) const override;

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Index

struct Index : AccessT {
  AccessHandle array;
  ExprHandle index;

  Index(AccessHandle &&arr, ExprHandle &&idx)
      : array(std::move(arr)), index(std::move(idx)) {}

  Access::Kind kind() const override { return Access::Kind::Index; }

  std::string to_string(size_t indent) const override;
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

} // namespace Access
} // namespace AST
