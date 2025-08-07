#include "Fmt.hpp"

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
  case AST::Stmt::Kind::Declaration: {
    name = "";
  } break;
  }

  return std::formatter<string_view>::format(name, ctx);
}

auto std::formatter<AST::Expr::Kind>::format(AST::Expr::Kind c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {

  case AST::Expr::Kind::Call: {
    name = "call";
  } break;
  case AST::Expr::Kind::Cast: {
    name = "cast";
  } break;
  case AST::Expr::Kind::CstI: {
    name = "csti";
  } break;
  case AST::Expr::Kind::Index: {
    name = "index";
  } break;
  case AST::Expr::Kind::Prim1: {
    name = "prim1";
  } break;
  case AST::Expr::Kind::Prim2: {
    name = "prim2";
  } break;
  case AST::Expr::Kind::Var: {
    name = "var";
  } break;
  }

  return std::formatter<string_view>::format(name, ctx);
}

auto std::formatter<AST::Typ::Kind>::format(AST::Typ::Kind c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {

  case AST::Typ::Kind::Bool: {
    name = "bool";
  } break;
  case AST::Typ::Kind::Char: {
    name = "char";
  } break;
  case AST::Typ::Kind::Int: {
    name = "int";
  } break;
  case AST::Typ::Kind::Ptr: {
    name = "ptr";
  } break;
  case AST::Typ::Kind::Void: {
    name = "void";
  } break;
  }
  return std::formatter<string_view>::format(name, ctx);
}

auto std::formatter<AST::Expr::OpUnary>::format(AST::Expr::OpUnary c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {

  case AST::Expr::OpUnary::AddressOf: {
    name = "&";
  } break;
  case AST::Expr::OpUnary::Dereference: {
    name = "*";
  } break;
  case AST::Expr::OpUnary::Sub: {
    name = "-";
  } break;
  case AST::Expr::OpUnary::Negation: {
    name = "!";
  } break;
  }
  return std::formatter<string_view>::format(name, ctx);
}

auto std::formatter<AST::Expr::OpBinary>::format(AST::Expr::OpBinary c, std::format_context &ctx) const
    -> std::format_context::iterator {
  string_view name = "unknown";
  switch (c) {

  case AST::Expr::OpBinary::Assign: {
    name = "=";
  } break;
  case AST::Expr::OpBinary::AssignAdd: {
    name = "+=";
  } break;
  case AST::Expr::OpBinary::AssignSub: {
    name = "-=";
  } break;
  case AST::Expr::OpBinary::AssignMul: {
    name = "*=";
  } break;
  case AST::Expr::OpBinary::AssignDiv: {
    name = "/=";
  } break;
  case AST::Expr::OpBinary::AssignMod: {
    name = "%=";
  } break;
  case AST::Expr::OpBinary::Add: {
    name = "+";
  } break;
  case AST::Expr::OpBinary::Sub: {
    name = "-";
  } break;
  case AST::Expr::OpBinary::Mul: {
    name = "*";
  } break;
  case AST::Expr::OpBinary::Div: {
    name = "/";
  } break;
  case AST::Expr::OpBinary::Mod: {
    name = "%";
  } break;
  case AST::Expr::OpBinary::Eq: {
    name = "==";
  } break;
  case AST::Expr::OpBinary::Neq: {
    name = "!=";
  } break;
  case AST::Expr::OpBinary::Gt: {
    name = ">";
  } break;
  case AST::Expr::OpBinary::Lt: {
    name = "<";
  } break;
  case AST::Expr::OpBinary::Leq: {
    name = "<=";
  } break;
  case AST::Expr::OpBinary::Geq: {
    name = ">=";
  } break;
  case AST::Expr::OpBinary::And: {
    name = "&&";
  } break;
  case AST::Expr::OpBinary::Or: {
    name = "||";
  } break;
  }
  return std::formatter<string_view>::format(name, ctx);
}
