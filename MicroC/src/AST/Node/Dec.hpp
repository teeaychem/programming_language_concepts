#pragma once

#include <string>

#include "AST/AST.hpp"

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
        typ(typ), id(name) {}

  llvm::Value *codegen(Context &ctx) const override;
  std::string to_string(size_t indent = 0) const override;

  Dec::Kind kind() const override { return Dec::Kind::Var; }
  TypHandle type() const override { return typ; };
  std::string name() const override { return this->id; };
};

// Prototype

struct Prototype : DecT {
private:
  TypHandle r_typ;
  std::string id;

public:
  VarTypVec args;

  Prototype(TypHandle r_typ, std::string name, VarTypVec args)
      : r_typ(r_typ),
        id(name),
        args(args) {}

  llvm::Value *codegen(Context &ctx) const override;
  std::string to_string(size_t indent = 0) const override;

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return r_typ; };
  std::string name() const override { return this->id; };

  TypHandle return_type() const { return r_typ; };
};

// Fn

struct Fn : DecT {
  PrototypeHandle prototype;
  Stmt::BlockHandle body;

  Fn(PrototypeHandle pt, Stmt::BlockHandle body)
      : prototype(pt),
        body(body) {}

  llvm::Value *codegen(Context &ctx) const override;
  std::string to_string(size_t indent = 0) const override;

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  TypHandle type() const override { return prototype->return_type(); };
  std::string name() const override { return this->prototype->name(); };

  TypHandle return_type() const { return prototype->return_type(); };
};

} // namespace Dec

} // namespace AST
