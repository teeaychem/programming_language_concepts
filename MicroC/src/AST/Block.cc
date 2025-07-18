#include "Block.hh"
#include "AST/AST.hh"
#include "Driver.hh"
#include <iostream>
#include <memory>

void AST::Block::push_DecVar(Driver &driver, AST::DecVarHandle &dec_var) {

  std::string var{dec_var->id};

  auto shadowed = driver.env.find(var);

  if (shadowed != driver.env.end()) {
    std::cout << "Shadowing: " << var << "\n";
    this->shadow_vars.push_back(dec_var);

    this->shadowed_vars.push_back(std::static_pointer_cast<Dec::Var>(shadowed->second));
  } else {
    std::cout << "Fresh: " << var << "\n";
    this->fresh_vars.push_back(dec_var);
  }

  driver.env[var] = dec_var;
}

void AST::Block::push_Stmt(AST::StmtHandle &stmt) {
  switch (stmt->kind()) {
  case Stmt::Kind::Block: {
    auto &stmt_block = std::static_pointer_cast<AST::Stmt::Block>(stmt)->block;
    this->early_returns += stmt_block.early_returns;
    this->fall_throughs += stmt_block.fall_throughs;
  } break;
  case Stmt::Kind::Return: {
    this->early_returns += 1;
    returns = true;
  }
  default:
    break;
  }

  this->statements.push_back(stmt);
}

void AST::Block::finalize(Driver &driver) {
  for (auto &shadowed : this->shadowed_vars) {
    std::cout << "Unshadowing: " << shadowed->name() << "\n";
    driver.env[shadowed->name()] = shadowed;
  }

  for (auto &fresh : this->fresh_vars) {
    std::cout << "Erasing: " << fresh->id << " ... ";
    driver.env.erase(fresh->id);
    std::cout << "OK" << "\n";
  }

  if (!this->returns) {
    this->fall_throughs += 1;
  }
}
