#pragma once

#include <optional>
#include <string>
#include <vector>

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"

#include "AST/Fmt.hpp"
#include "AST/Types.hpp"
#include "codegen/LLVMBundle.hpp"

#include "parser.hpp"

// Give flex the prototype of yylex
#define YY_DECL yy::parser::symbol_type yylex(Driver &drv)
YY_DECL; // Declare the prototype for bison

struct Driver {
  std::vector<AST::Stmt::DeclarationHandle> prg{};

  // Temporary storage for shadowed globals during parsing
  AST::VarTypMap shadow_cache{};

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
  void push_dec(AST::Stmt::DeclarationHandle stmt);

  // Handling the scanner.
  void scan_begin();
  void scan_end();

  // Retrun a string representation of the parsed program in 'canonical' form.
  // Useful for simple insight into how the AST was parsed.
  std::string prg_string();

  // etc

  void add_to_env(AST::VarTypVec &args) {
    for (auto &arg : args) {
      auto existing_global = this->llvm.env_ast.vars.find(arg.var);
      if (existing_global != this->llvm.env_ast.vars.end()) {
        this->shadow_cache[existing_global->first] = existing_global->second;
      }

      this->llvm.env_ast.vars[arg.var] = arg.typ;
    }
  }

  void fn_finalise(AST::Dec::FnHandle fn) {
    for (auto &param : fn->prototype->args) {
      this->llvm.env_ast.vars.erase(param.var);
    }

    for (auto &shadowed : this->shadow_cache) {
      this->llvm.env_ast.vars[shadowed.first] = shadowed.second;
    }

    this->shadow_cache.clear();
  }

  // representation

  AST::Expr::OpUnary to_unary_op(std::string op);
  AST::Expr::OpBinary to_binary_op(std::string op);

  // types

  // Returns the type which results from applying `op` to `expr`.
  AST::TypHandle type_resolution_prim1(AST::Expr::OpUnary op, AST::ExprHandle expr) {

    switch (op) {

    case AST::Expr::OpUnary::AddressOf: {
      return AST::Typ::pk_Ptr(expr->type(), std::nullopt);
    } break;

    case AST::Expr::OpUnary::Dereference: {
      if (expr->typ_has_kind(AST::Typ::Kind::Ptr)) {
        return expr->type()->deref();
      } else {
        throw std::logic_error(std::format("Deref panic... {} {}",
                                           expr->to_string(), expr->type()->to_string()));
      }
    } break;

    case AST::Expr::OpUnary::Sub: {
      return AST::Typ::pk_Int();
    } break;

    case AST::Expr::OpUnary::Negation: {
      return AST::Typ::pk_Bool();
    } break;
    }
  }

  void type_ensure_assignment(AST::ExprHandle lhs, AST::ExprHandle rhs) {

    // TODO: Unify this pattern, it likely appears elsewhere
    auto rhs_type = rhs->type();
    if (rhs->kind() == AST::Expr::Kind::Index) {
      auto as_index = std::static_pointer_cast<AST::Expr::Index>(rhs);
      rhs_type = as_index->target->type()->deref();
    }

    if ((lhs->typ_has_kind(rhs_type->kind())) //
        || (lhs->typ_has_kind(AST::Typ::Kind::Ptr) && lhs->type()->deref()->kind() == rhs_type->kind())) {
      return;
    }

    else {
      throw std::logic_error(std::format("Conflicting types for {}: {} and {}: {}",
                                         lhs->to_string(),
                                         lhs->type()->to_string(),
                                         rhs->to_string(),
                                         rhs_type->to_string()));
    }
  }

  void type_ensure_match(AST::ExprHandle lhs, AST::ExprHandle rhs) {
    if (lhs->typ_has_kind(rhs->type_kind())) {
      return;
    }

    if (lhs->type_kind() != rhs->type_kind()) {
      throw std::logic_error(std::format("Conflicting types for {}: {} and {}: {}",
                                         lhs->to_string(),
                                         lhs->type()->to_string(),
                                         rhs->to_string(),
                                         rhs->type()->to_string()));
    }
  }

  // Throws an error detailing an unsupported operation `op` on `lhs` and `rhs`.
  // A TypHandle is 'returned' in order to help lint failure to return a value in control paths which call this method.
  AST::TypHandle type_unsupported_binary_op(AST::Expr::OpBinary op, AST::ExprHandle lhs, AST::ExprHandle rhs) {
    throw std::logic_error(std::format("Unsupported operation {} on {} and {}",
                                       op,
                                       lhs->type()->to_string(),
                                       rhs->type()->to_string()));
  }

  AST::TypHandle type_resolution_prim2_ptr_expr(AST::Expr::OpBinary op, AST::ExprHandle ptr, AST::ExprHandle expr) {
    if (expr->typ_has_kind(AST::Typ::Kind::Int)) {
      return ptr->type();
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
      type_ensure_assignment(lhs, rhs);

      return rhs->type();
    } break;

    case AST::Expr::OpBinary::Add:
    case AST::Expr::OpBinary::Sub:
    case AST::Expr::OpBinary::Mul:
    case AST::Expr::OpBinary::Div:
    case AST::Expr::OpBinary::Mod: {
      if (lhs->type_kind() == rhs->type_kind()) {

        switch (lhs->type_kind()) {

        case AST::Typ::Kind::Bool:
        case AST::Typ::Kind::Char: {
          return type_unsupported_binary_op(op, lhs, rhs);
        } break;

        case AST::Typ::Kind::Int: {
          return AST::Typ::pk_Int();
        } break;

        case AST::Typ::Kind::Ptr:
        case AST::Typ::Kind::Void: {
          return type_unsupported_binary_op(op, lhs, rhs);
        } break;
        }

      }

      else if (lhs->typ_has_kind(AST::Typ::Kind::Ptr)) {
        return type_resolution_prim2_ptr_expr(op, lhs, rhs);
      }

      else if (rhs->typ_has_kind(AST::Typ::Kind::Ptr)) {
        return type_resolution_prim2_ptr_expr(op, rhs, lhs);
      }

      else {

        throw std::logic_error(std::format("todo: type resolution: {} {}",
                                           lhs->type()->to_string(),
                                           rhs->type()->to_string()));
      }
    } break;

    case AST::Expr::OpBinary::Eq:
    case AST::Expr::OpBinary::Neq:
    case AST::Expr::OpBinary::Gt:
    case AST::Expr::OpBinary::Lt:
    case AST::Expr::OpBinary::Leq:
    case AST::Expr::OpBinary::Geq: {
      type_ensure_match(lhs, rhs);

      return AST::Typ::pk_Bool();
    } break;

    case AST::Expr::OpBinary::And:
    case AST::Expr::OpBinary::Or: {

      return AST::Typ::pk_Bool();
    } break;
    }
  }

  // pk start

  // Methods for creating nodes.
  // Esp. used during parsing.
  // `pk` stands for `pointer_make`, as each method returns a (shared) pointer to the node created.

  // pk Dec

  // Function declaration requires an existing prototype and a body.
  AST::Dec::FnHandle pk_DecFn(AST::Dec::PrototypeHandle prototype, AST::Stmt::BlockHandle body);

  // Prototypes require specification of return type, var, and arguments (as type var pairs).
  AST::Dec::PrototypeHandle pk_Prototype(AST::TypHandle r_typ, std::string var, AST::VarTypVec params);

  // Variable declaration requires specification scope, typ, and var of the variable.
  AST::Dec::VarHandle pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var);

  // pk Expr

  // Calls requires the var of the fn and arguments.
  // An error is thrown if no prototype is found, or if arguments are incorrect.
  // Overloads are provided for convenience.
  AST::Expr::CallHandle pk_ExprCall(std::string name, std::vector<AST::ExprHandle> params);
  AST::Expr::CallHandle pk_ExprCall(std::string name, AST::ExprHandle param);
  AST::Expr::CallHandle pk_ExprCall(std::string name);

  // Casts require the expression cast and the target type.
  AST::Expr::CastHandle pk_ExprCast(AST::ExprHandle expr, AST::TypHandle to);

  // Ints are 64 bit throughout.
  AST::Expr::CstIHandle pk_ExprCstI(std::int64_t i);

  //
  AST::Expr::IndexHandle pk_ExprIndex(AST::ExprHandle access, AST::ExprHandle index);

  AST::Expr::Prim1Handle pk_ExprPrim1(AST::Expr::OpUnary op, AST::ExprHandle expr);

  AST::Expr::Prim2Handle pk_ExprPrim2(AST::Expr::OpBinary op, AST::ExprHandle a, AST::ExprHandle b);

  AST::Expr::VarHandle pk_ExprVar(std::string var);

  // pk Stmt

  AST::Stmt::BlockHandle pk_StmtBlock(AST::Block &&block);

  AST::Stmt::BlockHandle pk_StmtBlockStmt(AST::Block &&block);

  AST::Stmt::DeclarationHandle pk_StmtDeclaration(AST::DecHandle declaration);

  AST::Stmt::ExprHandle pk_StmtExpr(AST::ExprHandle expr);

  AST::Stmt::IfHandle pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle thn, AST::StmtHandle els);

  AST::Stmt::ReturnHandle pk_StmtReturn(std::optional<AST::ExprHandle> value);

  AST::Stmt::WhileHandle pk_StmtWhile(AST::ExprHandle condition, AST::StmtHandle block);

  // pk end
};
