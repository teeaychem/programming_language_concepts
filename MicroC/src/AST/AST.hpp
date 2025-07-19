#pragma once

#include <cstdio>
#include <memory>
#include <string>

#include "codegen.hpp"

// Types

namespace AST {

namespace Typ {

enum class Kind {
  Arr,
  Data,
  Ptr,
};

enum class Data {
  Int,
  Char,
  Void,
};

struct TypIndex;
struct TypData;
struct TypPtr;

} // namespace Typ

struct TypT {
  // Generate the representation of this type.
  [[nodiscard]] virtual llvm::Type *typegen(LLVMBundle &hdl) = 0;
  // Generate the default value of this type.
  [[nodiscard]] virtual llvm::Constant *defaultgen(LLVMBundle &hdl) const = 0;
  // The kind of this type, corresponding to a struct
  [[nodiscard]] virtual Typ::Kind kind() const = 0;
  // String representation, C-style
  [[nodiscard]] virtual std::string to_string(size_t indent) const = 0;
  // Dereference this type, may panic if dereference is not possible.
  [[nodiscard]] virtual std::shared_ptr<TypT> deref_unsafe() const = 0;
  // Completes the type, may panic if already complete.
  virtual void complete_data_unsafe(AST::Typ::Data d_typ) = 0;

  virtual ~TypT() = default;
};

typedef std::shared_ptr<TypT> TypHandle;
typedef std::shared_ptr<Typ::TypIndex> TypIndexHandle;
} // namespace AST

// Nodes

namespace AST {

enum class Kind {
  Access,
  Expr,
  Stmt,
  Dec,
};

struct NodeT {
  virtual llvm::Value *codegen(LLVMBundle &hdl) = 0;

  [[nodiscard]] virtual AST::Kind kind_abstract() const = 0;
  [[nodiscard]] virtual std::string to_string(size_t indent) const = 0;

  virtual ~NodeT() = default;
};
} // namespace AST

// Nodes

namespace AST {

// Access

namespace Access {

enum class Kind {
  Deref,
  Index,
  Var,
};

struct Deref;
struct Index;
struct Var;

} // namespace Access

struct AccessT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Access; }
  [[nodiscard]] virtual Access::Kind kind() const = 0;
  [[nodiscard]] virtual AST::TypHandle eval_type() const = 0;
};

// Expressions

namespace Expr {
struct Access;
struct Assign;
struct Call;
struct CstI;
struct Prim1;
struct Prim2;

enum class Kind {
  Access, // Access, Addr
  Assign,
  Call,
  CstI,
  Prim1,
  Prim2
};
} // namespace Expr

struct ExprT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Expr; }
  [[nodiscard]] virtual Expr::Kind kind() const = 0;
  [[nodiscard]] virtual AST::TypHandle type() const = 0;
};

// Statements

namespace Stmt {

enum class Kind {
  Block,
  Expr,
  If,
  Return,
  While
};

struct Block;
struct Expr;
struct If;
struct Return;
struct While;
} // namespace Stmt

struct StmtT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Stmt; }
  [[nodiscard]] virtual Stmt::Kind kind() const = 0;
  [[nodiscard]] virtual bool returns() const = 0;
  [[nodiscard]] virtual size_t early_returns() const = 0;
  [[nodiscard]] virtual size_t pass_throughs() const = 0;
};

// Declarations

namespace Dec {
enum class Kind {
  Var,
  Fn,
};

struct Var;
struct Fn;
} // namespace Dec

struct DecT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Dec; }
  [[nodiscard]] virtual Dec::Kind kind() const = 0;
  [[nodiscard]] virtual AST::TypHandle type() const = 0;
  [[nodiscard]] virtual std::string name() const = 0;
};
} // namespace AST

// etc.

namespace AST {

struct Block;

// Handles

typedef std::shared_ptr<AccessT> AccessHandle;
typedef std::shared_ptr<Access::Index> AccessIndexHandle;

typedef std::shared_ptr<DecT> DecHandle;

typedef std::shared_ptr<Dec::Var> DecVarHandle;
typedef std::shared_ptr<Dec::Fn> DecFnHandle;

typedef std::shared_ptr<ExprT> ExprHandle;
typedef std::shared_ptr<StmtT> StmtHandle;
typedef std::shared_ptr<AST::Stmt::Block> StmtBlockHandle;

// Etc

typedef std::vector<std::pair<TypHandle, std::string>> ParamVec;
typedef std::vector<std::variant<StmtHandle, DecHandle>> BlockVec;

typedef std::map<std::string, AST::DecHandle> Env;

} // namespace AST
