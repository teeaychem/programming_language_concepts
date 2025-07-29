#include "AST.hpp"

#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include <format>
#include <memory>
#include <stdexcept>

// Expr

void AST::Expr::Assign::type_resolution(Env &env) {
  this->dest->type_resolution(env);
  this->expr->type_resolution(env);

  this->typ = this->dest->typ;
}

void AST::Expr::Call::type_resolution(Env &env) {
  auto it = env.find(this->name);
  if (it == env.end()) {
    throw std::logic_error(std::format("Call for `{}`, but not found in env", this->name));
  }

  this->typ = env[this->name];
}

void AST::Expr::CstI::type_resolution(Env &env) { return; }

void AST::Expr::Index::type_resolution(Env &env) { return; }

void AST::Expr::Prim1::type_resolution(Env &env) { return; }

void AST::Expr::Prim2::type_resolution(Env &env) { return; }

void AST::Expr::Var::type_resolution(Env &env) { return; }

// Stmt

void AST::Stmt::Block::type_resolution(Env &env) {

  for (auto &fresh : this->block.fresh_vars) {
    fresh->type_resolution(env);
  }

  for (auto &shadowed : this->block.shadowed_vars) {
    // shadowed->type_resolution(env);
  }

  for (auto &stmt : this->block.statements) {
    stmt->type_resolution(env);
  }

  // Restore shadowed variables to `env`
  for (auto &shadowed : this->block.shadowed_vars) {
    // env[shadowed->name()] = shadowed->type();
  }

  // Remove fresh variables from `env`
  for (auto &fresh : this->block.fresh_vars) {
    env.erase(fresh->declaration->name());
  }

  return;
}

// Type resolution for declarations.
void AST::Stmt::Declaration::type_resolution(Env &env) {
  this->declaration->type_resolution(env);

  switch (this->declaration->kind()) {

  case Dec::Kind::Var: {
    env[this->declaration->name()] = this->declaration->type();
  } break;
  case Dec::Kind::Fn:

    auto as_fn = std::static_pointer_cast<AST::Dec::Fn>(this->declaration);

    env[as_fn->name()] = as_fn->r_typ;
    for (auto &param : as_fn->params) {
      env[param.first] = param.second;
    }

    break;
  }
}

void AST::Stmt::Expr::type_resolution(Env &env) {
  this->expr->type_resolution(env);
}

void AST::Stmt::If::type_resolution(Env &env) {
  this->condition->type_resolution(env);
  this->stmt_then->type_resolution(env);
  this->stmt_else->type_resolution(env);
}

void AST::Stmt::Return::type_resolution(Env &env) {
  if (this->value.has_value()) {
    this->value.value()->type_resolution(env);
  }
}

void AST::Stmt::While::type_resolution(Env &env) {
  this->condition->type_resolution(env);
  this->body->type_resolution(env);
}

// Dec

void AST::Dec::Var::type_resolution(Env &env) {
  if (!this->typ) {
    throw std::logic_error(std::format("Variable '{}' declared without a type", this->id));
  }
}

void AST::Dec::Fn::type_resolution(Env &env) {
  if (!this->r_typ) {
    throw std::logic_error(std::format("Fn '{}' declared without a return type", this->id));
  }
  for (auto &param : this->params) {
    if (!param.second) {
      throw std::logic_error(std::format("Fn declared without a type for parameter {}", param.first));
    }
  }
}
