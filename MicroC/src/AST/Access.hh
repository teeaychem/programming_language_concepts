#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
#include <memory>
#include <string>

#include "AST/AST.hh"

namespace AST {
namespace Access {

// Var

struct Var : AccessT {
  std::string var;

  Var(std::string &&v) : var(std::move(v)) {}

  std::string to_string() const override { return fmt::format("(Var {})", var); }
  Access::Kind kind() const override { return Access::Kind::Var; }

  const Access::Var *as_Var() const & { return this; }
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
  std::string to_string() const override {
    return fmt::format("(Deref *)", expr->to_string());
  }

  const Deref *as_Deref() const & { return this; }
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
  std::string to_string() const override {
    return fmt::format("(Index {} {})", array->to_string(),
                       index->to_string());
  }

  const Index *as_Index() const & { return this; }
};

inline AccessHandle pk_Index(AccessHandle arr, ExprHandle idx) {
  Index access(std::move(arr), std::move(idx));
  return std::make_shared<Index>(std::move(access));
}

} // namespace Access
} // namespace AST
