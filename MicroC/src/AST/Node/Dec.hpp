#pragma once

#include <string>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

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
      : scope(scope),
        typ(std::move(typ)), id(name) {}

  Dec::Kind kind() const override { return Dec::Kind::Var; }
  TypHandle type() const override { return typ; };
  std::string name() const override { return this->id; };

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Prototype

struct Prototype : DecT {
  TypHandle r_typ;
  std::string id;

  ParamVec params;

  Prototype(TypHandle r_typ, std::string name, ParamVec params)
      : r_typ(std::move(r_typ)),
        id(name),
        params(std::move(params)) {}

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return r_typ; };
  std::string name() const override { return this->id; };
  TypHandle return_type() const { return r_typ; };

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

// Fn

struct Fn : DecT {
  PrototypeHandle prototype;
  StmtBlockHandle body;

  Fn(PrototypeHandle prototype, StmtBlockHandle body)
      : prototype(std::move(prototype)),
        body(std::move(body)) {}

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return prototype->r_typ; };
  std::string name() const override { return this->prototype->id; };
  TypHandle return_type() const { return prototype->r_typ; };

  llvm::Value *codegen(LLVMBundle &hdl) const override;
  std::string to_string(size_t indent) const override;
};

} // namespace Dec

} // namespace AST
