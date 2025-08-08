#pragma once

#include <format>

#include "AST.hpp"

template <>
struct std::formatter<AST::Stmt::Kind> : formatter<string_view> {
  auto format(AST::Stmt::Kind c, format_context &ctx) const -> format_context::iterator;
};

template <>
struct std::formatter<AST::Expr::Kind> : formatter<string_view> {
  auto format(AST::Expr::Kind c, format_context &ctx) const -> format_context::iterator;
};

template <>
struct std::formatter<AST::Typ::Kind> : formatter<string_view> {
  auto format(AST::Typ::Kind c, format_context &ctx) const -> format_context::iterator;
};

template <>
struct std::formatter<AST::Expr::OpUnary> : formatter<string_view> {
  auto format(AST::Expr::OpUnary op, format_context &ctx) const -> format_context::iterator;
};

template <>
struct std::formatter<AST::Expr::OpBinary> : formatter<string_view> {
  auto format(AST::Expr::OpBinary op, format_context &ctx) const -> format_context::iterator;
};
