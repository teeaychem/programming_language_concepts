#pragma once

#include "AST.hpp"
#include <format>

template <>
struct std::formatter<AST::Typ::Data> : formatter<string_view> {
  auto format(AST::Typ::Data c, format_context &ctx) const -> format_context::iterator;
};

template <>
struct std::formatter<AST::Stmt::Kind> : formatter<string_view> {
  auto format(AST::Stmt::Kind c, format_context &ctx) const -> format_context::iterator;
};
