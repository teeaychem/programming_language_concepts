

#include "Block.hpp"
#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Stmt.hpp"

void AST::Block::push_DecVar(AST::Env &env, AST::DecVarHandle const &dec_var) {
  std::string var{dec_var->id};

  auto shadowed = env.find(var);

  if (shadowed != env.end()) {
    this->shadow_vars.push_back(dec_var);

    this->shadowed_vars.push_back(std::static_pointer_cast<Dec::Var>(shadowed->second));
  } else {
    this->fresh_vars.push_back(dec_var);
  }

  env[var] = dec_var;
}

void AST::Block::push_Stmt(AST::StmtHandle const &stmt) {
  switch (stmt->kind()) {

  case Stmt::Kind::Block: {
    this->early_returns += stmt->early_returns();
    this->pass_throughs += stmt->pass_throughs();

    if (!this->returns) {
      this->returns = stmt->returns();
    }
  } break;

  case Stmt::Kind::Return: {
    this->early_returns += 1;

    if (!this->returns) {
      this->scoped_return = true;
    }

    this->returns = true;

  } break;

  case Stmt::Kind::If: {
    auto stmt_if = std::static_pointer_cast<AST::Stmt::If>(stmt);
    size_t passed_through = this->pass_throughs;

    this->early_returns += stmt_if->stmt_then->early_returns();
    this->pass_throughs += stmt_if->stmt_then->pass_throughs();

    this->early_returns += stmt_if->stmt_else->early_returns();
    this->pass_throughs += stmt_if->stmt_else->pass_throughs();

    if (!this->returns) {
      this->returns = stmt_if->stmt_then->returns() && stmt_if->stmt_else->returns();
    }

  } break;

  case Stmt::Kind::While: {
    auto stmt_while = std::static_pointer_cast<AST::Stmt::While>(stmt);
    this->early_returns += stmt_while->stmt->early_returns();
    this->pass_throughs += stmt_while->stmt->pass_throughs();

    this->returns = stmt_while->returns();
  } break;

  default:
    break;
  }

  this->statements.push_back(stmt);
}

void AST::Block::finalize(AST::Env &env) {
  // Restore shadowed variables to `env`
  for (auto &shadowed : this->shadowed_vars) {
    env[shadowed->name()] = shadowed;
  }

  // Remove fresh variables from `env`
  for (auto &fresh : this->fresh_vars) {
    env.erase(fresh->id);
  }

  if (!this->returns) {
    this->pass_throughs += 1;
  }
}
