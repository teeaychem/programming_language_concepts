#pragma once

#include <memory>
#include <string>

#include "AST/AST.hpp"
#include "codegen/LLVMBundle.hpp"

namespace AST {
namespace Access {

// Var

struct Var : AccessT {
  std::string var;
  TypHandle typ;

  Var(TypHandle typ, std::string &&v) : typ(typ), var(std::move(v)) {}

  std::string to_string(size_t indent) const override;
  Access::Kind kind() const override { return Access::Kind::Var; }
  TypHandle eval_type() const override { return typ; }

  llvm::Value *codegen(LLVMBundle &hdl) const override;
};




} // namespace Access
} // namespace AST
