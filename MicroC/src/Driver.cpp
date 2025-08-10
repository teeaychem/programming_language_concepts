#include "Driver.hpp"

#include "AST/AST.hpp"
#include "AST/Node/Dec.hpp"
#include "AST/Node/Expr.hpp"
#include "AST/Node/Stmt.hpp"
#include "codegen/Structs.hpp"

void Driver::generate_ir() {
  for (auto &dec : prg) {
    dec->codegen(ctx);
  }
};

int Driver::parse(const std::string &file) {

  // Ensure a fresh env.
  assert(this->env.empty());

  src_file = file;
  location.initialize(&src_file);
  int res;

  scan_begin();
  yy::parser parse(*this);
  parse.set_debug_level(trace_parsing);
  res = parse();
  scan_end();

  // Do not clear the env as it contains fns and globals
  return res;
}

std::string Driver::prg_string() {
  std::string prg_str{};
  prg_str.append("\n");

  for (auto &dec : prg) {
    prg_str.append(dec->to_string());
    prg_str.append("\n");
  }

  return prg_str;
}

void Driver::print_llvm() {
  printf("\n----------\n");
  ctx.module->print(llvm::outs(), nullptr);
  printf("\n----------\n");
}

void Driver::push_dec(AST::Stmt::DeclarationHandle stmt) { prg.push_back(stmt); }

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

// Pointer make methods for declarations

// Global declarations are added to the AST environment when made.
// The motivation is recusive fn calls, which require access to the fn prototype.
// This motivation is extended to vars to form a rule.

AST::Dec::FnHandle Driver::pk_DecFn(AST::Dec::PrototypeHandle prototype, AST::Stmt::BlockHandle body) {
  if (!this->ctx.env_ast.fns.contains(prototype->name())) {
    throw std::logic_error(std::format("Missing prototype for {}", prototype->name()));
  }

  AST::Dec::Fn dec(std::move(prototype), std::move(body));
  return std::make_shared<AST::Dec::Fn>(std::move(dec));
}

AST::Dec::PrototypeHandle Driver::pk_Prototype(AST::TypHandle r_typ, std::string var, AST::VarTypVec args) {
  if (this->ctx.env_ast.fns.find(var) != this->ctx.env_ast.fns.end()) {
    throw std::logic_error(std::format("Existing prototype for: {}.", var));
  }

  AST::Dec::Prototype prototype(std::move(r_typ), var, std::move(args));
  auto pt_ptr = std::make_shared<AST::Dec::Prototype>(std::move(prototype));
  this->ctx.env_ast.fns[var] = pt_ptr;

  return pt_ptr;
}

AST::Dec::VarHandle Driver::pk_DecVar(AST::Dec::Scope scope, AST::TypHandle typ, std::string var) {
  if (scope == AST::Dec::Scope::Global) {
    if (this->ctx.env_ast.vars.find(var) != this->ctx.env_ast.vars.end()) {
      throw std::logic_error(std::format("Redeclaration of global: {}", var));
    }

    this->ctx.env_ast.vars[var] = typ;
  }

  AST::Dec::Var dec(scope, std::move(typ), var);
  return std::make_shared<AST::Dec::Var>(std::move(dec));
}

// Pointer make methods for expressions

AST::Expr::CallHandle Driver::pk_ExprCall(std::string var, std::vector<AST::ExprHandle> args) {

  auto prototype_find = this->ctx.env_ast.fns.find(var);
  if (prototype_find == this->ctx.env_ast.fns.end()) {
    throw std::logic_error(std::format("Call without prototype: {}", var));
  }
  auto prototype = prototype_find->second;

  if (args.size() != prototype->args.size()) {
    throw std::logic_error(std::format("Call to '{}' expected {} args, found {}",
                                       var,
                                       prototype->args.size(),
                                       args.size()));
  }

  for (int i = 0; i < args.size(); ++i) {

    auto arg_prototype = prototype->args[i].typ;

    if (!args[i]->typ_has_kind(arg_prototype->kind())) {

      auto arg_access_type = this->ctx.access_type(args[i].get());
      if (arg_prototype->kind() != arg_access_type->kind()) {
        auto cast = pk_ExprCast(args[i], arg_prototype);
        args[i] = cast;
      }
    }
  }

  AST::Expr::Call call(std::move(prototype->return_type()), std::move(var), std::move(args));

  return std::make_shared<AST::Expr::Call>(std::move(call));
}

AST::Expr::CastHandle Driver::pk_ExprCast(AST::ExprHandle expr, AST::TypHandle to) {

  AST::Expr::Cast cast(expr, to);
  return std::make_shared<AST::Expr::Cast>(std::move(cast));
}

AST::Expr::CallHandle Driver::pk_ExprCall(std::string name, AST::ExprHandle param) {

  std::vector<AST::ExprHandle> params = std::vector<AST::ExprHandle>{param};
  return this->pk_ExprCall(name, params);
}

AST::Expr::CallHandle Driver::pk_ExprCall(std::string name) {

  std::vector<AST::ExprHandle> empty_params = std::vector<AST::ExprHandle>{};
  return this->pk_ExprCall(name, empty_params);
}

AST::Expr::CstIHandle Driver::pk_ExprCstI(std::int64_t i) {
  auto typ = AST::Typ::pk_Int();
  AST::Expr::CstI csti(std::move(typ), i);

  return std::make_shared<AST::Expr::CstI>(std::move(csti));
}

AST::Expr::IndexHandle Driver::pk_ExprIndex(AST::ExprHandle access, AST::ExprHandle index) {

  AST::Expr::Index instance(std::move(access), std::move(index));
  return std::make_shared<AST::Expr::Index>(std::move(instance));
}

AST::Expr::Prim1Handle Driver::pk_ExprPrim1(AST::Expr::OpUnary op, AST::ExprHandle expr) {

  auto typ = this->type_resolution_prim1(op, expr);
  AST::Expr::Prim1 prim1(std::move(typ), op, std::move(expr));

  return std::make_shared<AST::Expr::Prim1>(std::move(prim1));
}

AST::Expr::Prim2Handle Driver::pk_ExprPrim2(AST::Expr::OpBinary op, AST::ExprHandle lhs, AST::ExprHandle rhs) {

  auto typ = this->type_resolution_prim2(op, lhs, rhs);
  AST::Expr::Prim2 prim2(std::move(typ), op, std::move(lhs), std::move(rhs));
  return std::make_shared<AST::Expr::Prim2>(std::move(prim2));
}

AST::Expr::VarHandle Driver::pk_ExprVar(std::string var) {
  auto env_var = this->ctx.env_ast.vars.find(var);

  if (env_var == this->ctx.env_ast.vars.end()) {
    throw std::logic_error(std::format("Unknown variable: {}", var));
  }

  auto typ = env_var->second;
  AST::Expr::Var access(std::move(typ), std::move(var));
  return std::make_shared<AST::Expr::Var>(std::move(access));
}

// Pointer make methods for statements

AST::Stmt::BlockHandle Driver::pk_StmtBlock(AST::Block &&block) {
  AST::Stmt::Block stmt(std::move(block));
  return std::make_shared<AST::Stmt::Block>(std::move(stmt));
}

AST::Stmt::BlockHandle Driver::pk_StmtBlockStmt(AST::Block &&block) {
  AST::Stmt::Block stmt(std::move(block));
  return std::make_shared<AST::Stmt::Block>(std::move(stmt));
}

AST::Stmt::DeclarationHandle Driver::pk_StmtDeclaration(AST::DecHandle declaration) {
  AST::Stmt::Declaration stmt(std::move(declaration));
  return std::make_shared<AST::Stmt::Declaration>(std::move(stmt));
}

AST::Stmt::ExprHandle Driver::pk_StmtExpr(AST::ExprHandle expr) {
  AST::Stmt::Expr stmt(std::move(expr));
  return std::make_shared<AST::Stmt::Expr>(std::move(stmt));
}

AST::Stmt::IfHandle Driver::pk_StmtIf(AST::ExprHandle condition, AST::StmtHandle thn, AST::StmtHandle els) {

  AST::Stmt::BlockHandle block_then;
  AST::Stmt::BlockHandle block_else;

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

AST::Stmt::ReturnHandle Driver::pk_StmtReturn(std::optional<AST::ExprHandle> value) {
  AST::Stmt::Return stmt(std::move(value));
  return std::make_shared<AST::Stmt::Return>(std::move(stmt));
}

AST::Stmt::WhileHandle Driver::pk_StmtWhile(AST::ExprHandle condition, AST::StmtHandle block) {
  AST::Stmt::While stmt(condition, block);
  return std::make_shared<AST::Stmt::While>(std::move(stmt));
}

// pk end
