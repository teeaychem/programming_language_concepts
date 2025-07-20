#pragma once

#include "AST/AST.hpp"
#include "AST/Node/Access.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

#include "codegen/LLVMBundle.hpp"
#include "parser.hpp"

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
    // TODO: Tidy
    this->env[dec->name()] = dec;
    prg.push_back(dec);
  }

  void scan_begin(); // Handling the scanner.
  void scan_end();

  std::string prg_string();

  // pk start

  // pk Access

  AST::AccessHandle pk_AccessVar(AST::TypHandle typ, std::string var) {
    if (this->env.find(var) == this->env.end()) {
      std::cerr << "Unknown variable: " << var << std::endl;
    }

    AST::Access::Var access(std::move(typ), std::move(var));
    return std::make_shared<AST::Access::Var>(std::move(access));
  }

  AST::AccessHandle pk_AccessDeref(AST::ExprHandle expr) {
    AST::Access::Deref access(std::move(expr));
    return std::make_shared<AST::Access::Deref>(std::move(access));
  }

  AST::AccessIndexHandle pk_AccessIndex(AST::AccessHandle arr, AST::ExprHandle idx) {
    AST::Access::Index access(std::move(arr), std::move(idx));
    return std::make_shared<AST::Access::Index>(std::move(access));
  }

  // pk Dec

  AST::DecVarHandle pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var) {

    if (scope == AST::Dec::Scope::Global) {
      if (this->env.find(var) != this->env.end()) {
        std::cerr << "Redeclaration of global: " << var << "\n";
        exit(-1);
      }
    }

    AST::Dec::Var dec(scope, std::move(typ), var);
    return std::make_shared<AST::Dec::Var>(std::move(dec));
  }

  AST::DecHandle pk_DecFn(AST::TypHandle r_typ, std::string var, AST::ParamVec params, AST::StmtBlockHandle body) {

    if (this->env.find(var) != this->env.end()) {
      std::cerr << "Existing use of: '" << var << "' unable to declare function." << std::endl;
      exit(-1);
    }

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

    auto dec = this->env.find(name);
    if (dec == this->env.end() || dec->second->kind() != AST::Dec::Kind::Fn) {
      std::cerr << "Failed to find fn: " << name << std::endl;
      exit(-1);
    }

    auto as_FnDec = std::static_pointer_cast<AST::Dec::Fn>(dec->second);

    AST::Expr::Call e(std::move(name), as_FnDec->r_typ, std::move(params));

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

  AST::StmtBlockHandle pk_StmtBlock(AST::Block &&bv) {
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
