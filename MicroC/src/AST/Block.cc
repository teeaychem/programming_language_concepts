#include "Block.hh"
#include "AST/AST.hh"
#include "Driver.hh"
#include <iostream>

void AST::Block::push_DecVar(Driver &driver, AST::DecVarHandle &dec_var) {
  std::string var{dec_var->var};

  auto shadowed = driver.env.find(var);

  if (shadowed != driver.env.end()) {
    std::cout << "Shadowing: " << var << "\n";
    this->shadow_vars.push_back(dec_var);
    this->shadowed_vars.push_back(*shadowed);
  } else {
    std::cout << "Fresh: " << var << "\n";
    this->fresh_vars.push_back(dec_var);
  }

  driver.env[var] = dec_var;
}

void AST::Block::push_Stmt(AST::StmtHandle &stmt) {
  this->statements.push_back(stmt);
}

void AST::Block::finalize(Driver &driver) {
  for (auto &shadowed : this->shadowed_vars) {
    std::cout << "Unshadowing: " << shadowed.first << "\n";
    driver.env[shadowed.first] = shadowed.second;
  }
}
