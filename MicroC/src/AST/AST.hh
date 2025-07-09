#pragma once

#include <fmt/format.h>
#include <memory>
#include <string>

#include "CodegenLLVM.hh"

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

struct TypArr;
struct TypData;
struct TypPtr;

} // namespace Typ

struct TypT {
  [[nodiscard]] virtual Typ::Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;

  const Typ::TypArr *as_TypArr() const & { return nullptr; }
  const Typ::TypPtr *as_TypPtr() const & { return nullptr; }
  const Typ::TypData *as_TypData() const & { return nullptr; }

  virtual void complete_data(AST::Typ::Data d_typ) = 0;

  virtual ~TypT() = default;
};

struct NodeT {
  virtual llvm::Value *codegen(LLVMBundle &hdl) = 0;

  [[nodiscard]] virtual std::string to_string() const = 0;

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
  [[nodiscard]] virtual Access::Kind kind() const = 0;

  const Access::Deref *as_Deref() const & { return nullptr; }
  const Access::Index *as_Index() const & { return nullptr; }
  const Access::Var *as_Var() const & { return nullptr; }
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
  [[nodiscard]] virtual Expr::Kind kind() const = 0;

  const Expr::Access *as_Access() const & { return nullptr; }
  const Expr::Assign *as_Assign() const & { return nullptr; }
  const Expr::Call *as_Call() const & { return nullptr; }
  const Expr::CstI *as_CstI() const & { return nullptr; }
  const Expr::Prim1 *as_Prim1() const & { return nullptr; }
  const Expr::Prim2 *as_Prim2() const & { return nullptr; }
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
  [[nodiscard]] virtual Stmt::Kind kind() const = 0;

  const Stmt::Block *as_Block() const & { return nullptr; }
  const Stmt::Expr *as_Expr() const & { return nullptr; }
  const Stmt::If *as_If() const & { return nullptr; }
  const Stmt::Return *as_Return() const & { return nullptr; }
  const Stmt::While *as_While() const & { return nullptr; }
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
  [[nodiscard]] virtual Dec::Kind kind() const = 0;

  const Dec::Var *as_Var() const & { return nullptr; }
  const Dec::Fn *as_Fn() const & { return nullptr; }
};
} // namespace AST

// etc.

namespace AST {

// Handles

typedef std::shared_ptr<AccessT> AccessHandle;
typedef std::shared_ptr<DecT> DecHandle;
typedef std::shared_ptr<ExprT> ExprHandle;
typedef std::shared_ptr<StmtT> StmtHandle;
typedef std::shared_ptr<TypT> TypHandle;

// Etc

typedef std::vector<std::pair<TypHandle, std::string>> ParamVec;
typedef std::vector<std::variant<StmtHandle, DecHandle>> BlockVec;

} // namespace AST
