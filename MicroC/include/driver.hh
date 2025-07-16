#pragma once

#include "AST/AST.hh"
#include "AST/Node/Access.hh"
#include "AST/Node/Dec.hh"
#include "AST/Node/Expr.hh"
#include "AST/Node/Stmt.hh"
#include "CodegenLLVM.hh"
#include "parser.hh"

#include <memory>
#include <optional>
#include <string>

// Give flex the prototype of yylex
#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL; // Declare the prototype for bison

struct Driver {
  std::vector<AST::DecHandle> prg{};

  std::map<std::string, AST::DecHandle> env{};

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

  void push_dec(AST::DecHandle dec) {
    prg.push_back(dec);
  }

  void scan_begin(); // Handling the scanner.
  void scan_end();

  std::string prg_string();

  // pk start

  // pk Access

  AST::AccessHandle pk_AccessVar(std::string var) {
    auto details = this->env.find(var);

    if (details == this->env.end()) {
      std::cerr << "Unknow variable: " << var << "\n";
    }

    AST::Access::Var access(std::move(var));
    return std::make_shared<AST::Access::Var>(std::move(access));
  }

  AST::AccessHandle pk_AccessDeref(AST::ExprHandle expr) {
    AST::Access::Deref access(std::move(expr));
    return std::make_shared<AST::Access::Deref>(std::move(access));
  }

  AST::AccessHandle pk_AccessIndex(AST::AccessHandle arr, AST::ExprHandle idx) {
    AST::Access::Index access(std::move(arr), std::move(idx));
    return std::make_shared<AST::Access::Index>(std::move(access));
  }

  // pk Dec

  AST::DecVarHandle pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var) {
    AST::Dec::Var dec(scope, std::move(typ), var);

    return std::make_shared<AST::Dec::Var>(std::move(dec));
  }

  AST::DecHandle pk_DecFn(AST::TypHandle r_typ, std::string var, AST::ParamVec params, AST::BlockHandle body) {
    AST::Dec::Fn dec(std::move(r_typ), var, std::move(params), std::move(body));
    return std::make_shared<AST::Dec::Fn>(std::move(dec));
  }

  // pk Expr

  AST::ExprHandle pk_ExprAccess(AST::Expr::Access::Mode mode, AST::AccessHandle acc) {
    AST::Expr::Access e(mode, std::move(acc));
    return std::make_shared<AST::Expr::Access>(std::move(e));
  }

  AST::ExprHandle pk_ExprAssign(AST::AccessHandle dest, AST::ExprHandle expr) {
    AST::Expr::Assign e(dest, expr);
    return std::make_shared<AST::Expr::Assign>(std::move(e));
  }

  AST::ExprHandle pk_ExprCall(std::string name, std::vector<AST::ExprHandle> params) {
    AST::Expr::Call e(std::move(name), std::move(params));
    return std::make_shared<AST::Expr::Call>(std::move(e));
  }

  AST::ExprHandle pk_ExprCstI(std::int64_t i) {
    AST::Expr::CstI e(i);
    return std::make_shared<AST::Expr::CstI>(std::move(e));
  }

  AST::ExprHandle pk_ExprPrim1(std::string op, AST::ExprHandle expr) {
    AST::Expr::Prim1 e(op, std::move(expr));
    return std::make_shared<AST::Expr::Prim1>(std::move(e));
  }

  AST::ExprHandle pk_ExprPrim2(std::string op, AST::ExprHandle a, AST::ExprHandle b) {
    AST::Expr::Prim2 e(op, std::move(a), std::move(b));
    return std::make_shared<AST::Expr::Prim2>(std::move(e));
  }

  // pk Stmt

  AST::BlockHandle pk_StmtBlock(AST::Block &&bv) {
    AST::Stmt::Block b(std::move(bv));
    return std::make_shared<AST::Stmt::Block>(std::move(b));
  }

  AST::StmtHandle pk_StmtBlockStmt(AST::Block &&bv) {
    AST::Stmt::Block b(std::move(bv));
    return std::make_shared<AST::Stmt::Block>(std::move(b));
  }

  AST::StmtHandle pk_StmtExpr(AST::ExprHandle expr) {
    AST::Stmt::Expr e(std::move(expr));
    return std::make_shared<AST::Stmt::Expr>(std::move(e));
  }

  AST::StmtHandle pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle yes, AST::StmtHandle no) {
    AST::Stmt::If e(std::move(condition), std::move(yes), std::move(no));
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
