#pragma once

#include "AST/AST.hh"
#include <memory>
#include <string>

namespace AST {

namespace Dec {

// Var

enum class Scope {
  Local,
  Global
};

struct Var : DecT {
  Scope scope;
  TypHandle typ;
  std::string var;

  Var(Scope scope, TypHandle typ, std::string var)
      : scope(scope), typ(std::move(typ)), var(var) {}

  std::string to_string(size_t indent) const override;

  Dec::Kind kind() const override { return Dec::Kind::Var; }
  llvm::Value *codegen(LLVMBundle &hdl) override;
};

inline DecHandle pk_Var(Scope scope, TypHandle typ, std::string var) {
  Var dec(scope, std::move(typ), var);
  return std::make_shared<Var>(std::move(dec));
}

// Fn

struct Fn : DecT {
  TypHandle r_typ;
  std::string var;

  ParamVec params;

  AST::BlockVec body;

  Fn(TypHandle r_typ, std::string var, ParamVec params,
     BlockVec body)
      : r_typ(std::move(r_typ)),
        var(var),
        params(std::move(params)),
        body(std::move(body)) {}

  std::string to_string(size_t indent) const override;

  llvm::Value *codegen(LLVMBundle &hdl) override;

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
};

inline DecHandle pk_Fn(TypHandle r_typ, std::string var, ParamVec params, BlockVec body) {
  Fn dec(std::move(r_typ), var, std::move(params), std::move(body));
  return std::make_shared<Fn>(std::move(dec));
}

} // namespace Dec

} // namespace AST
