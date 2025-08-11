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

  // Code generation for a declaration.
  // Should always be called when a declaration is made, and always updates the env.
  // The details of shadowing are handled at block nodes.
  llvm::Value *codegen(Context &ctx) const override;

  std::string to_string(size_t indent = 0) const override;

  Dec::Kind kind() const override { return Dec::Kind::Var; }
  TypHandle type() const override { return typ; };
  std::string var() const override { return this->id; };
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
  std::string var() const override { return this->id; };

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
  std::string var() const override { return this->prototype->var(); };

  TypHandle return_type() const { return prototype->return_type(); };
};

} // namespace Dec

} // namespace AST
