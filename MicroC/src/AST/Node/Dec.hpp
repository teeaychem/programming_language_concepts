#pragma once

#include "AST/AST.hpp"
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
  std::string id;

  Var(Scope scope, TypHandle typ, std::string name)
      : scope(scope), typ(std::move(typ)), id(name) {}

  std::string to_string(size_t indent) const override;
  Dec::Kind kind() const override { return Dec::Kind::Var; }
  TypHandle type() const override { return typ; };
  std::string name() const override { return this->id; };

  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

// Fn

struct Fn : DecT {
  TypHandle r_typ;
  std::string id;

  ParamVec params;

  StmtBlockHandle body;

  Fn(TypHandle r_typ, std::string name, ParamVec params, StmtBlockHandle body)
      : r_typ(std::move(r_typ)),
        id(name),
        params(std::move(params)),
        body(std::move(body)) {}

  std::string to_string(size_t indent) const override;
  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return r_typ; };
  std::string name() const override { return this->id; };

  llvm::Value *codegen(LLVMBundle &hdl) const override;
};

} // namespace Dec

} // namespace AST
