#pragma once

#include "AST.hpp"

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

  bool returns{false};                                   // Whether all branches (including main) lead to a return statement
  bool scoped_return{false};                             // Whether the main branch leads to a return statement
  std::vector<AST::StmtDeclarationHandle> fresh_vars{};  // Variables whose name does *not* appear in any larger scope
  std::vector<AST::StmtDeclarationHandle> shadow_vars{}; // Variables whose name *does* appear in some larger scope

  std::vector<AST::StmtHandle> statements{};

  std::vector<AST::DecHandle> shadowed_vars{}; // (Temporary) storage of shadowed variables

  size_t early_returns{0}; // How many paths originating in the block *lead* to a return statment
  size_t pass_throughs{0}; // How many paths originating in the block *do not* lead to a return statment

  // Methods

  // Whether the block is empty (no declarations or statements).
  bool empty() const { return this->statements.empty() && this->fresh_vars.empty() && this->shadow_vars.empty(); };

  // Add a declaration, using `env` to determine which variables are in scope.
  // And, mutates `env` in accordance with the declaration.
  void push_DecVar(AST::Env &env, AST::StmtDeclarationHandle const &dec_var);

  // Add a statement.
  void push_Stmt(AST::StmtHandle const &stmt);

  // To be called after the final declaration / statement has been added to the block.
  // Of note, restores `env` to its state prior to processing the block.
  void finalize(AST::Env &env);
};

} // namespace AST
