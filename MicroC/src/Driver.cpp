#include <sstream>

#include "Driver.hpp"
#include "codegen/LLVMBundle.hpp"
#include "parser.hpp"

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"

void Driver::generate_llvm() {
  for (auto &dec : prg) {
    dec->codegen(llvm);
  }
};

int Driver::parse(const std::string &file) {

  // Ensure a fresh env.
  assert(this->env.empty());

  for (auto &foundation_elem : this->llvm.foundation_fn_map) {
    auto primative_fn = foundation_elem.second;
    this->env[primative_fn->name] = primative_fn->return_type;
  }

  src_file = file;
  location.initialize(&src_file);
  int res;
  {
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(trace_parsing);
    res = parse();
    scan_end();
  }

  // Clear the env as it holds no actionable information.
  this->env.clear();

  return res;
}

std::string Driver::prg_string() {
  std::stringstream prg_ss{};
  prg_ss << "\n\n";
  for (auto &dec : prg) {
    prg_ss << dec->to_string();
    prg_ss << "\n\n";
  }

  return prg_ss.str();
}

void Driver::print_llvm() {
  printf("\n----------\n");
  llvm.module->print(llvm::outs(), nullptr);
  printf("\n----------\n");
}

void Driver::push_dec(AST::StmtDeclarationHandle stmt) {

  switch (stmt->declaration->kind()) {

  case AST::Dec::Kind::Var: {
    this->env[stmt->declaration->name()] = stmt->declaration->type();
  } break;

  case AST::Dec::Kind::Fn: {
    auto as_fn = std::static_pointer_cast<AST::Dec::Fn>(stmt->declaration);
    if (!this->env.contains(as_fn->name())) {
      // throw std::logic_error(std::format("Missing prototype for {}", as_fn->name()));
      this->env[as_fn->name()] = as_fn->return_type();
    }
  } break;

  case AST::Dec::Kind::Prototype: {
    auto as_pt = std::static_pointer_cast<AST::Dec::Prototype>(stmt->declaration);
    this->env[as_pt->name()] = as_pt->return_type();
  } break;
  }

  prg.push_back(stmt);
}

AST::Expr::OpBinary Driver::to_binary_op(std::string op) {
  static std::map<std::string, AST::Expr::OpBinary> op_map{
      {"=", AST::Expr::OpBinary::Assign},
      {"+=", AST::Expr::OpBinary::AssignAdd},
      {"-=", AST::Expr::OpBinary::AssignSub},
      {"*=", AST::Expr::OpBinary::AssignMul},
      {"/=", AST::Expr::OpBinary::AssignDiv},
      {"%=", AST::Expr::OpBinary::AssignMod},
      {"+", AST::Expr::OpBinary::Add},
      {"-", AST::Expr::OpBinary::Sub},
      {"*", AST::Expr::OpBinary::Mul},
      {"/", AST::Expr::OpBinary::Div},
      {"%", AST::Expr::OpBinary::Mod},
      {"==", AST::Expr::OpBinary::Eq},
      {"!=", AST::Expr::OpBinary::Neq},
      {">", AST::Expr::OpBinary::Gt},
      {"<", AST::Expr::OpBinary::Lt},
      {"<=", AST::Expr::OpBinary::Leq},
      {">=", AST::Expr::OpBinary::Geq},
      {"&&", AST::Expr::OpBinary::And},
      {"||", AST::Expr::OpBinary::Or}

  };

  if (!op_map.contains(op)) {
    throw std::logic_error(std::format("Unrecognised binary op: {}", op));
  }

  return op_map[op];
}

AST::Expr::OpUnary Driver::to_unary_op(std::string op) {

  static std::map<std::string, AST::Expr::OpUnary> op_map{
      {"&", AST::Expr::OpUnary::AddressOf},
      {"*", AST::Expr::OpUnary::Dereference},
      {"-", AST::Expr::OpUnary::Sub},
      {"!", AST::Expr::OpUnary::Negation},
  };

  if (!op_map.contains(op)) {
    throw std::logic_error(std::format("Unrecognised unary op: {}", op));
  }

  return op_map[op];
}

// pk start

// TODO: Clean up moves, as these were implemented without much thought.

// pk typ
AST::TypHandle Driver::pk_Int() {
  AST::Typ::Int type_int{};
  return std::make_shared<AST::Typ::Int>(std::move(type_int));
};

AST::TypHandle Driver::pk_Char() {
  AST::Typ::Char type_char{};
  return std::make_shared<AST::Typ::Char>(std::move(type_char));
};

AST::TypHandle Driver::pk_Void() {
  AST::Typ::Void type_void{};
  return std::make_shared<AST::Typ::Void>(std::move(type_void));
}

AST::TypHandle Driver::pk_Ptr(AST::TypHandle typ, std::optional<std::int64_t> area) {
  AST::Typ::Ptr type_index(std::move(typ), std::move(area));
  return std::make_shared<AST::Typ::Ptr>(std::move(type_index));
}

AST::TypHandle Driver::pk_Ptr(AST::TypHandle typ) {
  AST::Typ::Ptr type_index(std::move(typ), std::nullopt);
  return std::make_shared<AST::Typ::Ptr>(std::move(type_index));
}

// pk Dec

AST::DecVarHandle Driver::pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var) {

  if (scope == AST::Dec::Scope::Global) {
    if (this->env.find(var) != this->env.end()) {
      throw std::logic_error(std::format("Redeclaration of global: {}", var));
    }
  }

  AST::Dec::Var dec(scope, std::move(typ), var);
  return std::make_shared<AST::Dec::Var>(std::move(dec));
}

AST::DecFnHandle Driver::pk_DecFn(AST::PrototypeHandle prototype, AST::StmtBlockHandle body) {

  AST::Dec::Fn fn(std::move(prototype), std::move(body));
  return std::make_shared<AST::Dec::Fn>(std::move(fn));
}

AST::PrototypeHandle Driver::pk_Prototype(AST::TypHandle r_typ, std::string var, AST::ParamVec params) {

  if (this->env.find(var) != this->env.end()) {
    throw std::logic_error(std::format("Existing use of: '{}' unable to declare function.", var));
  }

  AST::Dec::Prototype prototype(std::move(r_typ), var, std::move(params));
  return std::make_shared<AST::Dec::Prototype>(std::move(prototype));
}

// pk Expr

AST::ExprHandle Driver::pk_ExprCall(std::string name, std::vector<AST::ExprHandle> params) {

  auto r_typ = this->env[name];
  if (!r_typ) {
    throw std::logic_error(std::format("Creation of call without a return type: {}", name));
  }

  AST::Expr::Call call(std::move(r_typ), std::move(name), std::move(params));

  return std::make_shared<AST::Expr::Call>(std::move(call));
}

AST::ExprHandle Driver::pk_ExprCall(std::string name, AST::ExprHandle param) {

  std::vector<AST::ExprHandle> params = std::vector<AST::ExprHandle>{param};
  return this->pk_ExprCall(name, params);
}

AST::ExprHandle Driver::pk_ExprCall(std::string name) {

  std::vector<AST::ExprHandle> empty_params = std::vector<AST::ExprHandle>{};
  return this->pk_ExprCall(name, empty_params);
}

AST::ExprHandle Driver::pk_ExprCstI(std::int64_t i) {
  auto typ = this->pk_Int();
  AST::Expr::CstI csti(std::move(typ), i);

  return std::make_shared<AST::Expr::CstI>(std::move(csti));
}

AST::ExprHandle Driver::pk_ExprIndex(AST::ExprHandle access, AST::ExprHandle index) {

  AST::Expr::Index instance(std::move(access), std::move(index));
  return std::make_shared<AST::Expr::Index>(std::move(instance));
}

AST::ExprHandle Driver::pk_ExprPrim1(AST::Expr::OpUnary op, AST::ExprHandle expr) {

  auto typ = this->type_resolution_prim1(op, expr);
  AST::Expr::Prim1 prim1(std::move(typ), op, std::move(expr));

  return std::make_shared<AST::Expr::Prim1>(std::move(prim1));
}

AST::ExprHandle Driver::pk_ExprPrim2(AST::Expr::OpBinary op, AST::ExprHandle lhs, AST::ExprHandle rhs) {

  auto typ = this->type_resolution_prim2(op, lhs, rhs);
  AST::Expr::Prim2 prim2(std::move(typ), op, std::move(lhs), std::move(rhs));
  return std::make_shared<AST::Expr::Prim2>(std::move(prim2));
}

AST::ExprHandle Driver::pk_ExprVar(std::string var) {
  if (this->env.find(var) == this->env.end()) {
    throw std::logic_error(std::format("Unknown variable: {}", var));
  }
  auto typ = this->env[var];
  AST::Expr::Var access(std::move(typ), std::move(var));
  return std::make_shared<AST::Expr::Var>(std::move(access));
}

// pk Stmt

AST::StmtBlockHandle Driver::pk_StmtBlock(AST::Block &&block) {
  AST::Stmt::Block stmt(std::move(block));
  return std::make_shared<AST::Stmt::Block>(std::move(stmt));
}

AST::StmtHandle Driver::pk_StmtBlockStmt(AST::Block &&block) {
  AST::Stmt::Block stmt(std::move(block));
  return std::make_shared<AST::Stmt::Block>(std::move(stmt));
}

AST::StmtDeclarationHandle Driver::pk_StmtDeclaration(AST::DecHandle &&declaration) {
  AST::Stmt::Declaration stmt(std::move(declaration));
  return std::make_shared<AST::Stmt::Declaration>(std::move(stmt));
}

AST::StmtHandle Driver::pk_StmtExpr(AST::ExprHandle expr) {
  AST::Stmt::Expr stmt(std::move(expr));
  return std::make_shared<AST::Stmt::Expr>(std::move(stmt));
}

AST::StmtHandle Driver::pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle thn, AST::StmtHandle els) {

  AST::StmtBlockHandle block_then;
  AST::StmtBlockHandle block_else;

  if (thn->kind() == AST::Stmt::Kind::Block) {
    block_then = std::static_pointer_cast<AST::Stmt::Block>(thn);
  } else {
    auto fresh_block = AST::Block();
    fresh_block.push_Stmt(thn);
    block_then = Driver::pk_StmtBlock(std::move(fresh_block));
  }

  if (els->kind() == AST::Stmt::Kind::Block) {
    block_else = std::static_pointer_cast<AST::Stmt::Block>(els);
  } else {
    auto fresh_block = AST::Block();
    fresh_block.push_Stmt(els);
    block_else = Driver::pk_StmtBlock(std::move(fresh_block));
  }

  AST::Stmt::If stmt(std::move(condition), std::move(block_then), std::move(block_else));

  return std::make_shared<AST::Stmt::If>(std::move(stmt));
}

AST::StmtHandle Driver::pk_StmtReturn(std::optional<AST::ExprHandle> value) {
  AST::Stmt::Return stmt(std::move(value));
  return std::make_shared<AST::Stmt::Return>(std::move(stmt));
}

AST::StmtHandle Driver::pk_StmtWhile(AST::ExprHandle condition, AST::StmtHandle block) {
  AST::Stmt::While stmt(condition, block);
  return std::make_shared<AST::Stmt::While>(std::move(stmt));
}

// pk end
