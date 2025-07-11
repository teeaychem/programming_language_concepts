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

inline AccessHandle pk_Var(std::string var) {
  Var access(std::move(var));
  return std::make_shared<Var>(std::move(access));
}

// Deref

struct Deref : AccessT {
  ExprHandle expr;

  Deref(ExprHandle &&expr) : expr(std::move(expr)) {}

  Access::Kind kind() const override { return Access::Kind::Deref; }
  std::string to_string(size_t indent) const override;

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline AccessHandle pk_Deref(ExprHandle expr) {
  Deref access(std::move(expr));
  return std::make_shared<Deref>(std::move(access));
}

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

inline AccessHandle pk_Index(AccessHandle arr, ExprHandle idx) {
  Index access(std::move(arr), std::move(idx));
  return std::make_shared<Index>(std::move(access));
}

} // namespace Access
} // namespace AST
