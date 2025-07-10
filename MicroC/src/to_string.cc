#include <cstdio>
#include <fmt/base.h>
#include <fmt/format.h>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "AST/AST.hh"
#include "AST/Node/Access.hh"
#include "AST/Node/Dec.hh"
#include "AST/Node/Expr.hh"
#include "AST/Node/Stmt.hh"

#include "AST/Types.hh"

// Types

template <>
struct fmt::formatter<AST::Typ::Data> : formatter<string_view> {
  auto format(AST::Typ::Data c, format_context &ctx) const -> format_context::iterator;
};

auto fmt::formatter<AST::Typ::Data>::format(AST::Typ::Data c, fmt::format_context &ctx) const
    -> fmt::format_context::iterator {
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
    break;
  }
  return fmt::formatter<string_view>::format(name, ctx);
}

std::string AST::Typ::TypData::to_string() const {
  return fmt::format("{}", d_typ);
}

std::string AST::Typ::TypArr::to_string() const {
  return fmt::format("(TypArr {})", size.value_or(0));
}

std::string AST::Typ::TypPtr::to_string() const {
  return fmt::format("*{}", dest->to_string());
}

// Nodes

// Access

std::string AST::Access::Deref::to_string() const {
  return fmt::format("*{}", this->expr->to_string());
}

std::string AST::Access::Index::to_string() const {
  return fmt::format("(Index {} {})", this->array->to_string(), this->index->to_string());
}

std::string AST::Access::Var::to_string() const {
  return fmt::format("{}", this->var);
}

// Dec

std::string AST::Dec::Fn::to_string() const {
  std::stringstream fn_ss{};

  fn_ss << r_typ->to_string() << " ";
  fn_ss << " " << this->var << " ";

  std::string param_str{};
  {
    std::stringstream param_stream{};
    for (auto &p : params) {
      param_stream << p.first->to_string();
      param_stream << " ";
      param_stream << p.second;
      param_stream << ", ";
    }
    param_str = param_stream.str();

    if (params.size()) {
      param_str.pop_back();
      param_str.pop_back();
    }
  }

  fn_ss << "(" << param_str << ")";
  fn_ss << " ";
  fn_ss << "{" << "\n";

  auto v_str = [&fn_ss](const auto v) { fn_ss << v->to_string() << "\n"; };

  for (auto &part : body) {
    std::visit(v_str, part);
  }

  fn_ss << "}";

  return fn_ss.str();
}

std::string AST::Dec::Var::to_string() const {
  return fmt::format("{} {}", typ->to_string(), var);
}

// Expr

std::string AST::Expr::Access::to_string() const {
  return fmt::format("{}", this->acc->to_string());
}
std::string AST::Expr::Assign::to_string() const {
  return fmt::format("{} = {}", this->dest->to_string(), this->expr->to_string());
}
std::string AST::Expr::Call::to_string() const {
  std::stringstream ss{};
  for (auto &param : parameters) {
    ss << param->to_string();
    ss << " ";
  }
  std::string params = ss.str();
  params.pop_back();

  return fmt::format("{}({})", this->name, params);
}
std::string AST::Expr::CstI::to_string() const {
  return fmt::format("{}", i);
}
std::string AST::Expr::Prim1::to_string() const {
  return fmt::format("{}{}", op, expr->to_string());
}
std::string AST::Expr::Prim2::to_string() const {
  return fmt::format("({} {} {})", this->a->to_string(), this->op, this->b->to_string());
}

// Stmt

std::string AST::Stmt::Block::to_string() const {

  std::stringstream block_ss{};

  auto v_out = [&block_ss](const auto v) { block_ss << v->to_string() << "\n"; };

  for (auto &x : block) {
    std::visit(v_out, x);
  }

  return block_ss.str();
}
std::string AST::Stmt::Expr::to_string() const {
  return fmt::format("{};", expr->to_string());
}
std::string AST::Stmt::If::to_string() const {
  return "TODO: if";
}
std::string AST::Stmt::Return::to_string() const {
  std::stringstream r_ss{};

  r_ss << "return";
  if (this->value.has_value()) {
    r_ss << " " << this->value.value()->to_string();
  }
  r_ss << ";";

  return r_ss.str();
}
std::string AST::Stmt::While::to_string() const {
  return "TODO: while";
}
