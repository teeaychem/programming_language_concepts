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
  TypHandle type() const override { return typ; };

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

// Fn

struct Fn : DecT {
  TypHandle r_typ;
  std::string name;

  ParamVec params;

  BlockHandle body;

  Fn(TypHandle r_typ, std::string var, ParamVec params, BlockHandle body)
      : r_typ(std::move(r_typ)),
        name(var),
        params(std::move(params)),
        body(std::move(body)) {}

  std::string to_string(size_t indent) const override;
  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return r_typ; };

  llvm::Value *codegen(LLVMBundle &hdl) override;
};

} // namespace Dec

} // namespace AST
