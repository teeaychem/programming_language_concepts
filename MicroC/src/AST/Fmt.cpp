#include "Fmt.hpp"
#include "AST/Node/Expr.hpp"

auto std::formatter<AST::Typ::Data>::format(AST::Typ::Data c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {
  case AST::Typ::Data::Int: {
    name = "int";
  } break;
  case AST::Typ::Data::Char: {
    name = "char";
  } break;
  case AST::Typ::Data::Void: {
    name = "void";
  } break;
  }
  return std::formatter<string_view>::format(name, ctx);
}

auto std::formatter<AST::Stmt::Kind>::format(AST::Stmt::Kind c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {
  case AST::Stmt::Kind::Block: {
    name = "Block";
  } break;
  case AST::Stmt::Kind::Expr: {
    name = "Expr";
  } break;
  case AST::Stmt::Kind::If: {
    name = "If";
  } break;
  case AST::Stmt::Kind::Return: {
    name = "Return";
  } break;
  case AST::Stmt::Kind::While: {
    name = "While";
  } break;
  }
  return std::formatter<string_view>::format(name, ctx);
}

