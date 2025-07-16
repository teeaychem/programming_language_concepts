#pragma once

#include "AST.hh"

struct Driver; // Required forward dec

/*
  Blocks contain variable declarations and statements.
  Block do not contain functions as functions can only be delcared in global scope.

  When parsing it is useful to have information about a variable at hand.
  This information is stored in the env(ironment) of a driver.

  As variables are scoped, the env must be updated during parsing.
  To do so, variables are split into 'fresh' and 'shadowed' variants.
  -  Fresh vars are removed from the env when parsing escapes the block.
  -  Shadowed vars are replaced in the env by the var which was shadowed, with the shadowed vars stored separately.

  With this, the env is updated whenever a var is declared, and whenever a block escapes scope.
  Specifically, for bison, a block escapes scope when passed upwards, at which point the `finalize` method should be called.
 */
namespace AST {
struct Block {

  std::vector<AST::DecHandle> fresh_vars{};
  std::vector<AST::DecHandle> shadow_vars{};

  std::vector<AST::StmtHandle> statements{};

  std::vector<std::pair<std::string, AST::DecHandle>> shadowed_vars{};

  bool empty() { return this->statements.empty() && this->fresh_vars.empty() && this->shadowed_vars.empty(); };

  void push_DecVar(Driver &driver, AST::DecVarHandle &dec_var);

  void push_Stmt(AST::StmtHandle &stmt);

  void finalize(Driver &driver);
};

} // namespace AST
