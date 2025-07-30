#pragma once

#include <format>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"

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

  int parse(const std::string &file); // Run the parser on file; return 0 on success.

  void push_dec(AST::StmtDeclarationHandle stmt);

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
    for (auto &param : fn->prototype->params) {
      this->env.erase(param.first);
    }
  }

  // representation

  AST::Expr::OpUnary to_unary_op(std::string op);
  AST::Expr::OpBinary to_binary_op(std::string op);

  // types

  AST::TypHandle type_data_handle(AST::Typ::Data data) {

    switch (data) {

    case AST::Typ::Data::Int: {
      std::shared_ptr<AST::Typ::TypData> static type_int = std::make_shared<AST::Typ::TypData>(AST::Typ::TypData(AST::Typ::Data::Int));
      return type_int;
    } break;
    case AST::Typ::Data::Char: {
      std::shared_ptr<AST::Typ::TypData> static type_char = std::make_shared<AST::Typ::TypData>(AST::Typ::TypData(AST::Typ::Data::Char));
      return type_char;
    } break;
    case AST::Typ::Data::Void: {
      std::shared_ptr<AST::Typ::TypData> type_void = std::make_shared<AST::Typ::TypData>(AST::Typ::TypData(AST::Typ::Data::Void));
      return type_void;
    } break;
    }
  }

  AST::TypHandle type_resolution_prim1(AST::Expr::OpUnary op, AST::ExprHandle expr) {
    // FIXME: Complete

    return expr->type();
  }

  // pk start

  // pk typ

  AST::TypHandle pk_Data(AST::Typ::Data data);

  AST::TypHandle pk_Void();

  AST::TypHandle pk_Index(AST::TypHandle typ, std::optional<std::int64_t> size);

  AST::TypHandle pk_Ptr(AST::TypHandle of);

  // pk Dec

  AST::DecVarHandle pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var);

  AST::DecFnHandle pk_DecFn(AST::PrototypeHandle prototype, AST::StmtBlockHandle body);

  AST::PrototypeHandle pk_Prototype(AST::TypHandle r_typ, std::string var, AST::ParamVec params);

  // pk Expr

  AST::ExprHandle pk_ExprCall(std::string name, std::vector<AST::ExprHandle> params);
  AST::ExprHandle pk_ExprCall(std::string name, AST::ExprHandle param);
  AST::ExprHandle pk_ExprCall(std::string name);

  AST::ExprHandle pk_ExprCstI(std::int64_t i);

  AST::ExprHandle pk_ExprIndex(AST::ExprHandle access, AST::ExprHandle index);

  AST::ExprHandle pk_ExprPrim1(AST::Expr::OpUnary op, AST::ExprHandle expr);

  AST::ExprHandle pk_ExprPrim2(AST::Expr::OpBinary op, AST::ExprHandle a, AST::ExprHandle b);

  AST::ExprHandle pk_ExprVar(std::string var);

  // pk Stmt

  AST::StmtBlockHandle pk_StmtBlock(AST::Block &&block);

  AST::StmtHandle pk_StmtBlockStmt(AST::Block &&block);

  AST::StmtDeclarationHandle pk_StmtDeclaration(AST::DecHandle &&declaration);

  AST::StmtHandle pk_StmtExpr(AST::ExprHandle expr);

  AST::StmtHandle pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle thn, AST::StmtHandle els);

  AST::StmtHandle pk_StmtReturn(std::optional<AST::ExprHandle> value);

  AST::StmtHandle pk_StmtWhile(AST::ExprHandle condition, AST::StmtHandle block);

  // pk end
};
