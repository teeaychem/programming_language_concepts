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
}
