#include <sstream>
#include <string>

#include "AST/AST.hpp"

#include "AST/Block.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "AST/Types.hpp"

#include "AST/Fmt.hpp"

size_t const INDENT_SIZE = 2;

// Support

// Types
std::string AST::Typ::Bool::to_string(size_t indent) const { return "bool"; }

std::string AST::Typ::Char::to_string(size_t indent) const { return "char"; }

std::string AST::Typ::Int::to_string(size_t indent) const { return "int"; }

std::string AST::Typ::Ptr::to_string(size_t indent) const {
  if (this->area().has_value()) {
    return std::format("{}[{}]", this->pointee_type()->to_string(indent), this->area().value());
  } else {
    return std::format("*{}", this->pointee_type()->to_string(indent));
  }
}

std::string AST::Typ::Void::to_string(size_t indent) const { return "void"; }

// Nodes

// Dec

std::string AST::Dec::Fn::to_string(size_t indent) const {
  return std::format("{} {}", this->prototype->to_string(), this->body->to_string(indent));
}

std::string AST::Dec::Prototype::to_string(size_t indent) const {
  std::stringstream fn_ss{};

  fn_ss << std::format("{} {} (", r_typ->to_string(indent), this->id);

  if (!args.empty()) {
    for (auto &p : args) {
      fn_ss << std::format("{} {}, ", p.second->to_string(indent), p.first);
    }
    fn_ss.seekp(-2, std::ios_base::end);
  }

  fn_ss << ")";

  return fn_ss.str();
}

std::string AST::Dec::Var::to_string(size_t indent) const {
  return std::format("{} {}", typ->to_string(indent), id);
}

// Expr

std::string AST::Expr::Call::to_string(size_t indent) const {
  std::stringstream call_ss{};

  call_ss << this->name
          << "(";

  if (!arguments.empty()) {
    for (auto &param : arguments) {
      call_ss << param->to_string(indent) << ", ";
    }
    call_ss.seekp(-2, std::ios_base::end);
  }

  call_ss << ")";

  std::string call_str = call_ss.str();

  if (!arguments.empty()) {
    call_str.pop_back();
  }

  return call_str;
}

std::string AST::Expr::Cast::to_string(size_t indent) const {
  return std::format("({}){}", this->typ->to_string(), this->expr->to_string());
}

std::string AST::Expr::CstI::to_string(size_t indent) const {
  return std::format("{}", i);
}

std::string AST::Expr::Index::to_string(size_t indent) const {
  return std::format("{}[{}]", this->target->to_string(indent), this->index->to_string(indent));
}

std::string AST::Expr::Prim1::to_string(size_t indent) const {
  return std::format("{}{}", op, expr->to_string(indent));
}

std::string AST::Expr::Prim2::to_string(size_t indent) const {
  return std::format("({} {} {})", this->lhs->to_string(indent), this->op, this->rhs->to_string(indent));
}

std::string AST::Expr::Var::to_string(size_t indent) const {
  return this->var;
}

// Stmt

std::string AST::Stmt::Block::to_string(size_t indent) const {

  std::stringstream block_ss{};

  block_ss << "{" << "\n";

  size_t updated_offset = indent + INDENT_SIZE;
  auto variant_out = [&block_ss, &indent, &updated_offset](const auto v) {
    block_ss << std::string(updated_offset, ' ') << v->to_string(updated_offset) << "\n";
  };

  for (auto &fresh_var : block.fresh_vars) {
    variant_out(fresh_var);
  }

  for (auto &shadow_var : block.shadow_vars) {
    variant_out(shadow_var);
  }

  for (auto &stmt : block.statements) {
    variant_out(stmt);
  }

  block_ss << std::string(indent, ' ')
           << "}";

  return block_ss.str();
}

std::string AST::Stmt::Declaration::to_string(size_t indent) const {
  std::string expr_str = this->declaration->to_string(indent);
  expr_str.push_back(';');

  return expr_str;
}

std::string AST::Stmt::Expr::to_string(size_t indent) const {
  std::string expr_str = expr->to_string(indent);
  expr_str.push_back(';');

  return expr_str;
}

std::string AST::Stmt::If::to_string(size_t indent) const {
  std::stringstream if_ss{};
  if_ss << std::format("if {} {}", this->condition->to_string(indent), this->stmt_then->to_string(indent));

  if (this->stmt_else->kind() == AST::Stmt::Kind::Block) {
    auto as_block = std::static_pointer_cast<AST::Stmt::Block>(this->stmt_else);
    if (as_block->block.empty()) {
      goto complete_if_string;
    }
  }

  if_ss << std::format(" else {}", this->stmt_else->to_string(indent));

complete_if_string:

  return if_ss.str();
}

std::string AST::Stmt::Return::to_string(size_t indent) const {
  std::stringstream r_ss{};

  r_ss << "return";
  if (this->value.has_value()) {
    r_ss << " "
         << this->value.value()->to_string(indent);
  }
  r_ss << ";";

  return r_ss.str();
}
std::string AST::Stmt::While::to_string(size_t indent) const {

  std::stringstream while_ss{};
  while_ss << "while" << " "
           << this->condition->to_string(indent) << " "
           << this->body->to_string(indent);

  return while_ss.str();
}

// Env

std::string AST::EnvAST::to_string() {
  std::stringstream ss{};
  ss << "Env AST:" << "\n";
  for (auto &var : vars) {
    ss << "\t" << var.first << " : " << var.second->to_string() << "\n";
  }
  for (auto &fn : fns) {
    ss << "\t" << fn.first << " : " << " ..." << "\n";
  }

  return ss.str();
}
