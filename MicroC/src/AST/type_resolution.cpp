#include "AST.hpp"

#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

// Expr

void AST::Expr::Assign::type_resolution(Env &env) { return; }

void AST::Expr::Call::type_resolution(Env &env) { return; }

void AST::Expr::CstI::type_resolution(Env &env) { return; }

void AST::Expr::Index::type_resolution(Env &env) { return; }

void AST::Expr::Prim1::type_resolution(Env &env) { return; }

void AST::Expr::Prim2::type_resolution(Env &env) { return; }

void AST::Expr::Var::type_resolution(Env &env) { return; }

// Stmt

void AST::Stmt::Block::type_resolution(Env &env) { return; }

void AST::Stmt::Declaration::type_resolution(Env &env) { return; }

void AST::Stmt::Expr::type_resolution(Env &env) { return; }

void AST::Stmt::If::type_resolution(Env &env) { return; }

void AST::Stmt::Return::type_resolution(Env &env) { return; }

void AST::Stmt::While::type_resolution(Env &env) { return; }

// Dec

void AST::Dec::Var::type_resolution(Env &env) { return; }

void AST::Dec::Fn::type_resolution(Env &env) { return; }
