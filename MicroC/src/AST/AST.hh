#pragma once

#include <fmt/format.h>
#include <memory>
#include <string>

namespace AST {

// Typ

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

typedef std::shared_ptr<TypT> TypHandle;

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

struct AccessT {
  [[nodiscard]] virtual Access::Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;

  const Access::Deref *as_Deref() const & { return nullptr; }
  const Access::Index *as_Index() const & { return nullptr; }
  const Access::Var *as_Var() const & { return nullptr; }

  virtual ~AccessT() = default;
};

typedef std::shared_ptr<AccessT> AccessHandle;

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

struct ExprT {
  [[nodiscard]] virtual Expr::Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;

  const Expr::Access *as_Access() const & { return nullptr; }
  const Expr::Assign *as_Assign() const & { return nullptr; }
  const Expr::Call *as_Call() const & { return nullptr; }
  const Expr::CstI *as_CstI() const & { return nullptr; }
  const Expr::Prim1 *as_Prim1() const & { return nullptr; }
  const Expr::Prim2 *as_Prim2() const & { return nullptr; }

  virtual ~ExprT() = default;
};

typedef std::shared_ptr<ExprT> ExprHandle;

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

struct StmtT {
  [[nodiscard]] virtual Stmt::Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;

  const Stmt::Block *as_Block() const & { return nullptr; }
  const Stmt::Expr *as_Expr() const & { return nullptr; }
  const Stmt::If *as_If() const & { return nullptr; }
  const Stmt::Return *as_Return() const & { return nullptr; }
  const Stmt::While *as_While() const & { return nullptr; }

  virtual ~StmtT() = default;
};

typedef std::shared_ptr<StmtT> StmtHandle;

namespace Dec {
enum class Kind {
  Var,
  Fn,
};

struct Var;
struct Fn;
} // namespace Dec

struct DecT {
  [[nodiscard]] virtual Dec::Kind kind() const = 0;
  [[nodiscard]] virtual std::string to_string() const = 0;

  const Dec::Var *as_Var() const & { return nullptr; }
  const Dec::Fn *as_Fn() const & { return nullptr; }

  virtual ~DecT() = default;
};

typedef std::shared_ptr<DecT> DecHandle;

typedef std::vector<std::variant<StmtHandle, DecHandle>> BlockVec;
typedef std::vector<std::pair<TypHandle, std::string>> ParamVec;

} // namespace AST
