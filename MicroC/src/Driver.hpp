#pragma once

#include <optional>
#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"

#include "AST/Fmt.hpp"
#include "AST/Types.hpp"
#include "codegen/LLVMBundle.hpp"

#include "parser.hpp"

// Give flex the prototype of yylex
#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL; // Declare the prototype for bison

struct Driver {
  std::vector<AST::StmtDeclarationHandle> prg{};

  // Fn / Prototype / Variable to type mapping maintained during parsing.
  // Empty before and after parsing.
  Env env{};

  // A bundle of things useful for LLVM codegen.
  LLVMBundle llvm{};

  // The file to be parsed.
  std::string src_file;

  bool trace_parsing;
  bool trace_scanning;

  // Token location.
  yy::location location;

  Driver()
      : trace_parsing(false),
        trace_scanning(false),
        llvm(LLVMBundle{}) {}

  void generate_llvm();
  void print_llvm();

  // Run the parser on file; return 0 on success.
  int parse(const std::string &file);

  // Push a declaration to the AST representation of the program.
  void push_dec(AST::StmtDeclarationHandle stmt);

  // Handling the scanner.
  void scan_begin();
  void scan_end();

  // Retrun a string representation of the parsed program in 'canonical' form.
  // Useful for simple insight into how the AST was parsed.
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

      auto typ_int = AST::Typ::TypData(AST::Typ::Data::Int);
      std::shared_ptr<AST::Typ::TypData> static type_int = std::make_shared<AST::Typ::TypData>(typ_int);

      return type_int;
    } break;

    case AST::Typ::Data::Char: {
      auto typ_char = AST::Typ::TypData(AST::Typ::Data::Char);
      std::shared_ptr<AST::Typ::TypData> static type_char = std::make_shared<AST::Typ::TypData>(typ_char);

      return type_char;
    } break;

    case AST::Typ::Data::Void: {
      auto typ_void = AST::Typ::TypData(AST::Typ::Data::Void);
      std::shared_ptr<AST::Typ::TypData> type_void = std::make_shared<AST::Typ::TypData>(typ_void);

      return type_void;
    } break;
    }
  }

  // Returns the type which results from applying `op` to `expr`.
  AST::TypHandle type_resolution_prim1(AST::Expr::OpUnary op, AST::ExprHandle expr) {

    switch (op) {

    case AST::Expr::OpUnary::AddressOf: {
      return this->pk_Ptr(expr->type());
    } break;

    case AST::Expr::OpUnary::Dereference: {
      return expr->type()->deref();
    } break;

    case AST::Expr::OpUnary::Minus: {
      return expr->type();
    } break;

    case AST::Expr::OpUnary::Negation: {
      return expr->type();
    } break;
    }
  }

  void type_ensure_match(AST::ExprHandle lhs, AST::ExprHandle rhs) {
    if (lhs->typ->kind() != rhs->typ->kind()) {
      throw std::logic_error("Conflicting types");
    }
  }

  // Throws an error detailing an unsupported operation `op` on `lhs` and `rhs`.
  // A TypHandle is 'returned' in order to help lint failure to return a value in control paths which call this method.
  AST::TypHandle type_unsupported_binary_op(AST::Expr::OpBinary op, AST::ExprHandle lhs, AST::ExprHandle rhs) {
    throw std::logic_error(std::format("Unsupported operation {} on {} and {}", op, lhs->type()->to_string(0), rhs->type()->to_string(0)));
  }

  AST::TypHandle type_resolution_prim2_ptr_expr(AST::Expr::OpBinary op, AST::ExprHandle ptr, AST::ExprHandle expr) {
    if (expr->typ->kind() == AST::Typ::Kind::Data) {
      auto as_data = std::static_pointer_cast<AST::Typ::TypData>(expr->type());
      if (as_data->data == AST::Typ::Data::Int) {
        return ptr->typ;
      }
    }

    return type_unsupported_binary_op(op, ptr, expr);
  }

  AST::TypHandle type_resolution_prim2(AST::Expr::OpBinary op, AST::ExprHandle lhs, AST::ExprHandle rhs) {

    switch (op) {

    case AST::Expr::OpBinary::Assign:
    case AST::Expr::OpBinary::AssignAdd:
    case AST::Expr::OpBinary::AssignSub:
    case AST::Expr::OpBinary::AssignMul:
    case AST::Expr::OpBinary::AssignDiv:
    case AST::Expr::OpBinary::AssignMod: {
      type_ensure_match(lhs, rhs);

      return lhs->type();
    } break;

    case AST::Expr::OpBinary::Add:
    case AST::Expr::OpBinary::Sub:
    case AST::Expr::OpBinary::Mul:
    case AST::Expr::OpBinary::Div:
    case AST::Expr::OpBinary::Mod: {
      if (lhs->typ->kind() == rhs->typ->kind()) {
        if (lhs->type()->kind() == AST::Typ::Kind::Data) {
          auto as_data = std::static_pointer_cast<AST::Typ::TypData>(lhs->type());

          switch (as_data->data) {

          case AST::Typ::Data::Int:
          case AST::Typ::Data::Char: {
            return lhs->type();
          }

          case AST::Typ::Data::Void: {
            return type_unsupported_binary_op(op, lhs, rhs);
          } break;
          }

        }

        else {
          return type_unsupported_binary_op(op, lhs, rhs);
        }

      }

      else if (lhs->typ->kind() == AST::Typ::Kind::Pointer) {
        return type_resolution_prim2_ptr_expr(op, lhs, rhs);
      }

      else if (rhs->typ->kind() == AST::Typ::Kind::Pointer) {
        return type_resolution_prim2_ptr_expr(op, rhs, lhs);
      }

      else {
        throw std::logic_error("todo");
      }

    } break;

    case AST::Expr::OpBinary::Eq:
    case AST::Expr::OpBinary::Neq:
    case AST::Expr::OpBinary::Gt:
    case AST::Expr::OpBinary::Lt:
    case AST::Expr::OpBinary::Leq:
    case AST::Expr::OpBinary::Geq: {
      type_ensure_match(lhs, rhs);

      return type_data_handle(AST::Typ::Data::Int);
    } break;

    case AST::Expr::OpBinary::And:
    case AST::Expr::OpBinary::Or: {

      return type_data_handle(AST::Typ::Data::Int);
    } break;
    }
  }

  // pk start

  // Methods for creating nodes.
  // Esp. used during parsing.
  // `pk` stands for `pointer_make`, as each method returns a (shared) pointer to the node created.

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
