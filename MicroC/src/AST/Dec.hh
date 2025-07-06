#pragma once

#include "AST/AST.hh"
#include <fmt/format.h>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

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

  std::string to_string() const override { return fmt::format("(Dec {} {} {})", fmt::underlying(scope), typ->to_string(), var); }
  Dec::Kind kind() const override { return Dec::Kind::Var; }

  const Dec::Var *as_Var() const & { return this; }
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

  std::string to_string() const override {
    std::string param_str{};
    {
      std::stringstream param_stream{};
      for (auto &p : params) {
        param_stream << p.first->to_string();
        param_stream << " ";
        param_stream << p.second;
        param_stream << ", ";
      }
      param_str = param_stream.str();

      if (params.size()) {
        param_str.pop_back();
        param_str.pop_back();
      }
    }

    return fmt::format("(Fn {} {} [{}]) [{}]", r_typ->to_string(), var, param_str, body.size());
  }

  Dec::Kind kind() const override { return Dec::Kind::Fn; }
  const Dec::Fn *as_Fn() const & { return this; }
};

inline DecHandle pk_Fn(TypHandle r_typ, std::string var, ParamVec params, BlockVec body) {
  Fn dec(std::move(r_typ), var, std::move(params), std::move(body));
  return std::make_shared<Fn>(std::move(dec));
}

} // namespace Dec

} // namespace AST
