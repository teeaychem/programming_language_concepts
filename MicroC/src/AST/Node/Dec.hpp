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
private:
  std::string id;
  TypHandle typ;

public:
  Scope scope;

  Var(Scope scope, TypHandle typ, std::string name)
      : scope(scope),
        typ(std::move(typ)), id(name) {}

  Dec::Kind kind() const override { return Dec::Kind::Var; }
  TypHandle type() const override { return typ; };
  std::string name() const override { return this->id; };

  llvm::Value *codegen(LLVMBundle &bundle) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Prototype

struct Prototype : DecT {
private:
  TypHandle r_typ;
  std::string id;

public:
  VarTypVec args;

  Prototype(TypHandle r_typ, std::string name, VarTypVec args)
      : r_typ(std::move(r_typ)),
        id(name),
        args(std::move(args)) {}

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return r_typ; };
  std::string name() const override { return this->id; };
  TypHandle return_type() const { return r_typ; };

  llvm::Value *codegen(LLVMBundle &bundle) const override;
  std::string to_string(size_t indent = 0) const override;
};

// Fn

struct Fn : DecT {
  PrototypeHandle prototype;
  StmtBlockHandle body;

  Fn(PrototypeHandle prototype, StmtBlockHandle body)
      : prototype(std::move(prototype)),
        body(std::move(body)) {}

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return prototype->return_type(); };
  std::string name() const override { return this->prototype->name(); };
  TypHandle return_type() const { return prototype->return_type(); };

  llvm::Value *codegen(LLVMBundle &bundle) const override;
  std::string to_string(size_t indent = 0) const override;
};

} // namespace Dec

} // namespace AST
