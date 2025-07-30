#pragma once

#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "AST/Types.hpp"
#include "codegen/LLVMBundle.hpp"
#include "parser.hpp"

// Give flex the prototype of yylex
#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL; // Declare the prototype for bison

struct Driver {
  std::vector<AST::StmtDeclarationHandle> prg{};

  Env env{};

  LLVMBundle llvm;

  std::string file; // The file to be parsed
  bool trace_parsing;
  bool trace_scanning;
  yy::location location; // Token location.

  Driver()
      : trace_parsing(false),
        trace_scanning(false),
        llvm(LLVMBundle{}) {}

  void generate_llvm();
  void print_llvm();

  int parse(const std::string &f); // Run the parser on file F.  Return 0 on success.

  void push_dec(AST::StmtDeclarationHandle stmt) {

    switch (stmt->declaration->kind()) {

    case AST::Dec::Kind::Var: {
      this->env[stmt->declaration->name()] = stmt->declaration->type();
    } break;

    case AST::Dec::Kind::Fn: {
      auto as_fn = std::static_pointer_cast<AST::Dec::Fn>(stmt->declaration);
      this->env[as_fn->name()] = as_fn->return_type();
    } break;
    }

    prg.push_back(stmt);
  }

  void scan_begin(); // Handling the scanner.
  void scan_end();

  std::string prg_string();

  // etc

  void add_to_env(AST::ParamVec &params) {
    for (auto &param : params) {
      this->env[param.first] = param.second;
    }
  }

  void fn_finalise(AST::DecFnHandle fn) {
    // TODO: Shadowing of global variables...
    for (auto &param : fn->params) {
      this->env.erase(param.first);
    }
  }

  // pk start

  // pk Dec

  AST::DecVarHandle pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var) {

    if (scope == AST::Dec::Scope::Global) {
      if (this->env.find(var) != this->env.end()) {
        throw std::logic_error(std::format("Redeclaration of global: {}", var));
      }
    }

    AST::Dec::Var dec(scope, std::move(typ), var);
    return std::make_shared<AST::Dec::Var>(std::move(dec));
  }

  AST::DecFnHandle pk_DecFn(AST::TypHandle r_typ, std::string var, AST::ParamVec params, AST::StmtBlockHandle body) {

    if (this->env.find(var) != this->env.end()) {
      throw std::logic_error(std::format("Existing use of: '{}' unable to declare function.", var));
    }

    AST::Dec::Fn dec(std::move(r_typ), var, std::move(params), std::move(body));
    return std::make_shared<AST::Dec::Fn>(std::move(dec));
  }

  // pk Expr

  AST::ExprHandle pk_ExprAssign(AST::ExprHandle dest, AST::ExprHandle expr) {
    AST::Expr::Assign e(dest, expr);
    return std::make_shared<AST::Expr::Assign>(std::move(e));
  }

  AST::ExprHandle pk_ExprCall(std::string name, std::vector<AST::ExprHandle> params) {

    AST::Expr::Call e(std::move(name), std::move(params));

    return std::make_shared<AST::Expr::Call>(std::move(e));
  }

  AST::ExprHandle pk_ExprCall(std::string name, AST::ExprHandle param) {

    std::vector<AST::ExprHandle> params = std::vector<AST::ExprHandle>{param};

    AST::Expr::Call e(std::move(name), std::move(params));

    return std::make_shared<AST::Expr::Call>(std::move(e));
  }

  AST::ExprHandle pk_ExprCall(std::string name) {

    std::vector<AST::ExprHandle> empty_params = std::vector<AST::ExprHandle>{};

    AST::Expr::Call e(std::move(name), std::move(empty_params));

    return std::make_shared<AST::Expr::Call>(std::move(e));
  }

  AST::ExprHandle pk_ExprCstI(std::int64_t i) {
    AST::Expr::CstI e(i);
    return std::make_shared<AST::Expr::CstI>(std::move(e));
  }

  AST::ExprHandle pk_ExprIndex(AST::ExprHandle access, AST::ExprHandle index) {

    AST::Expr::Index instance(std::move(access), std::move(index));
    return std::make_shared<AST::Expr::Index>(std::move(instance));
  }

  AST::ExprHandle pk_ExprPrim1(std::string op, AST::ExprHandle expr) {
    AST::Expr::Prim1 e(op, std::move(expr));
    return std::make_shared<AST::Expr::Prim1>(std::move(e));
  }

  AST::ExprHandle pk_ExprPrim2(std::string op, AST::ExprHandle a, AST::ExprHandle b) {
    AST::Expr::Prim2 e(op, std::move(a), std::move(b));
    return std::make_shared<AST::Expr::Prim2>(std::move(e));
  }

  AST::ExprHandle pk_ExprVar(std::string var) {
    if (this->env.find(var) == this->env.end()) {
      throw std::logic_error(std::format("Unknown variable: {}", var));
    }
    auto tmp = AST::Typ::pk_Data(AST::Typ::Data::Int);

    AST::Expr::Var access(std::move(tmp), std::move(var));
    return std::make_shared<AST::Expr::Var>(std::move(access));
  }

  // pk Stmt

  AST::StmtBlockHandle pk_StmtBlock(AST::Block &&bv) {
    AST::Stmt::Block stmt(std::move(bv));
    return std::make_shared<AST::Stmt::Block>(std::move(stmt));
  }

  AST::StmtHandle pk_StmtBlockStmt(AST::Block &&bv) {
    AST::Stmt::Block stmt(std::move(bv));
    return std::make_shared<AST::Stmt::Block>(std::move(stmt));
  }

  AST::StmtDeclarationHandle pk_StmtDeclaration(AST::DecHandle &&declaration) {
    AST::Stmt::Declaration stmt(std::move(declaration));
    return std::make_shared<AST::Stmt::Declaration>(std::move(stmt));
  }

  AST::StmtHandle pk_StmtExpr(AST::ExprHandle expr) {
    AST::Stmt::Expr stmt(std::move(expr));
    return std::make_shared<AST::Stmt::Expr>(std::move(stmt));
  }

  AST::StmtHandle pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle yes, AST::StmtHandle no) {

    AST::StmtBlockHandle yes_block;
    AST::StmtBlockHandle no_block;

    if (yes->kind() == AST::Stmt::Kind::Block) {
      yes_block = std::static_pointer_cast<AST::Stmt::Block>(yes);
    } else {
      auto fresh_block = AST::Block();
      fresh_block.push_Stmt(yes);
      yes_block = pk_StmtBlock(std::move(fresh_block));
    }

    if (no->kind() == AST::Stmt::Kind::Block) {
      no_block = std::static_pointer_cast<AST::Stmt::Block>(no);
    } else {
      auto fresh_block = AST::Block();
      fresh_block.push_Stmt(no);
      no_block = pk_StmtBlock(std::move(fresh_block));
    }

    AST::Stmt::If e(std::move(condition), std::move(yes_block), std::move(no_block));
    return std::make_shared<AST::Stmt::If>(std::move(e));
  }

  AST::StmtHandle pk_StmtReturn(std::optional<AST::ExprHandle> value) {
    AST::Stmt::Return e(std::move(value));
    return std::make_shared<AST::Stmt::Return>(std::move(e));
  }

  AST::StmtHandle pk_StmtWhile(AST::ExprHandle condition, AST::StmtHandle bv) {
    AST::Stmt::While e(condition, bv);
    return std::make_shared<AST::Stmt::While>(std::move(e));
  }

  // pk end
};
