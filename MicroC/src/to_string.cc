#include <cstdio>
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

size_t const OFFSET = 2;

// Support

// Types

template <>
struct std::formatter<AST::Typ::Data> : formatter<string_view> {
  auto format(AST::Typ::Data c, format_context &ctx) const -> format_context::iterator;
};

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

std::string AST::Typ::TypData::to_string(size_t indent) const {
  return std::format("{}", d_typ);
}

std::string AST::Typ::TypArr::to_string(size_t indent) const {
  if (this->size.has_value()) {
    return std::format("{}[{}]", this->typ->to_string(indent), size.value());
  } else {
    return std::format("{}[]", this->typ->to_string(indent));
  }
}

std::string AST::Typ::TypPtr::to_string(size_t indent) const {
  return std::format("*{}", dest->to_string(indent));
}

// Nodes

// Access

std::string AST::Access::Deref::to_string(size_t indent) const {
  return std::format("*{}", this->expr->to_string(indent));
}

std::string AST::Access::Index::to_string(size_t indent) const {
  return std::format("{}[{}]", this->array->to_string(indent), this->index->to_string(indent));
}

std::string AST::Access::Var::to_string(size_t indent) const {
  return this->var;
}

// Dec

std::string AST::Dec::Fn::to_string(size_t indent) const {
  std::stringstream fn_ss{};

  fn_ss << r_typ->to_string(indent) << " ";
  fn_ss << " " << this->var << " ";

  std::string param_str{};
  {
    std::stringstream param_stream{};
    for (auto &p : params) {
      param_stream << p.first->to_string(indent);
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

  fn_ss << "(" << param_str << ")"
        << " "
        << "{" << "\n"
        << this->body->to_string(indent)
        << std::string(indent, ' ') << "}";

  return fn_ss.str();
}

std::string AST::Dec::Var::to_string(size_t indent) const {
  return std::format("{} {};", typ->to_string(indent), var);
}

// Expr

std::string AST::Expr::Access::to_string(size_t indent) const {
  return std::format("{}", this->acc->to_string(indent));
}
std::string AST::Expr::Assign::to_string(size_t indent) const {
  return std::format("{} = {}", this->dest->to_string(indent), this->expr->to_string(indent));
}
std::string AST::Expr::Call::to_string(size_t indent) const {
  std::stringstream ss{};
  for (auto &param : parameters) {
    ss << param->to_string(indent);
    ss << ", ";
  }
  std::string params = ss.str();
  params.pop_back();
  params.pop_back();

  return std::format("{}({})", this->name, params);
}
std::string AST::Expr::CstI::to_string(size_t indent) const {
  return std::format("{}", i);
}
std::string AST::Expr::Prim1::to_string(size_t indent) const {
  return std::format("{}{}", op, expr->to_string(indent));
}
std::string AST::Expr::Prim2::to_string(size_t indent) const {
  return std::format("({} {} {})", this->a->to_string(indent), this->op, this->b->to_string(indent));
}

// Stmt

std::string AST::Stmt::Block::to_string(size_t indent) const {

  std::stringstream block_ss{};
  block_ss << "{" << "\n";

  auto variant_out = [&block_ss, &indent](const auto v) {
    size_t updated_offset = indent + OFFSET;
    block_ss << std::string(updated_offset, ' ') << v->to_string(updated_offset) << "\n"; };

  for (auto &block_variant : block) {
    std::visit(variant_out, block_variant);
  }

  block_ss << std::string(indent, ' ') << "}";

  return block_ss.str();
}

std::string AST::Stmt::Expr::to_string(size_t indent) const {
  return std::format("{};", expr->to_string(indent));
}

std::string AST::Stmt::If::to_string(size_t indent) const {
  std::stringstream if_ss{};
  if_ss << "if"
        << " "
        << this->condition->to_string(indent)
        << " "
        << this->yes->to_string(indent);

  if (this->no->kind() == AST::Stmt::Kind::Block) {
    auto as_block = std::static_pointer_cast<AST::Stmt::Block>(this->no);
    if (!as_block->block.empty()) {
      if_ss << " else "
            << this->no->to_string(indent);
    }
  }

  return if_ss.str();
}

std::string AST::Stmt::Return::to_string(size_t indent) const {
  std::stringstream r_ss{};

  r_ss << "return";
  if (this->value.has_value()) {
    r_ss << " " << this->value.value()->to_string(indent);
  }
  r_ss << ";";

  return r_ss.str();
}
std::string AST::Stmt::While::to_string(size_t indent) const {

  std::stringstream while_ss{};
  while_ss << "while"
           << " "
           << this->condition->to_string(indent)
           << " "
           << this->stmt->to_string(indent);

  return while_ss.str();
}
