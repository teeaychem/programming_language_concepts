#pragma once

#include <cstdio>
#include <map>
#include <memory>
#include <string>

#include "llvm/IR/DIBuilder.h"
struct LLVMBundle;

// Types

namespace AST {

namespace Typ {

enum class Kind {
  Array,   // An array.
  Data,    // Some instance of Data (further specified).
  Pointer, // A pointer.
};

enum class Data {
  Int,  // An integer.
  Char, // A character.
  Void, // Unspecified, and requires completion to be a usable type.
};

struct TypIndex;
struct TypData;
struct TypPointer;

} // namespace Typ

struct TypT {
  // Generate the representation of this type.
  virtual llvm::Type *typegen(LLVMBundle &hdl) const = 0;
  // The kind of this type, corresponding to a struct.
  virtual Typ::Kind kind() const = 0;
  // String representation.
  virtual std::string to_string(size_t indent) const = 0;
  // Dereference this type, may panic if dereference is not possible.
  virtual std::shared_ptr<TypT> deref_unsafe() const = 0;
  // Completes the type, may throw if already complete.
  virtual void complete_data_unsafe(AST::Typ::Data d_typ) = 0;

  virtual ~TypT() = default;
};

typedef std::shared_ptr<TypT> TypHandle;
typedef std::shared_ptr<Typ::TypIndex> TypIndexHandle;

} // namespace AST

typedef std::map<std::string, AST::TypHandle> Env; // Vars have declared type, fns have return type.

// Nodes

namespace AST {

enum class Kind {
  Access,
  Expr,
  Stmt,
  Dec,
};

struct NodeT {
  virtual llvm::Value *codegen(LLVMBundle &hdl) const = 0;

  virtual AST::Kind kind_abstract() const = 0;
  virtual std::string to_string(size_t indent) const = 0;
  //

  virtual ~NodeT() = default;
};
} // namespace AST

// Nodes

// Expressions

namespace AST {

namespace Expr {
struct Assign;
struct Call;
struct CstI;
struct Index;
struct Prim1;
struct Prim2;
struct Var;

enum class Kind {
  Assign,
  Call,
  CstI,
  Index,
  Prim1,
  Prim2,
  Var,
};
} // namespace Expr

struct ExprT : NodeT {
  TypHandle typ{nullptr};

  AST::Kind kind_abstract() const override { return AST::Kind::Expr; }
  virtual Expr::Kind kind() const = 0;
  virtual AST::TypHandle type() const = 0;
  llvm::Value *codegen_eval_true(LLVMBundle &hdl) const;
  llvm::Value *codegen_eval_false(LLVMBundle &hdl) const;
};

// Statements

namespace Stmt {

enum class Kind {
  Block,
  Declaration,
  Expr,
  If,
  Return,
  While
};

struct Block;
struct Declaration;
struct Expr;
struct If;
struct Return;
struct While;
} // namespace Stmt

struct StmtT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Stmt; }
  virtual Stmt::Kind kind() const = 0;
  virtual bool returns() const = 0;
  virtual size_t early_returns() const = 0;
  virtual size_t pass_throughs() const = 0;
};

// Declarations

namespace Dec {
enum class Kind {
  Var,
  Fn,
  Prototype,
};

struct Var;
struct Fn;
struct Prototype;
} // namespace Dec

struct DecT : NodeT {
  AST::Kind kind_abstract() const override { return AST::Kind::Dec; }
  virtual Dec::Kind kind() const = 0;
  virtual AST::TypHandle type() const = 0;
  virtual std::string name() const = 0;
};
} // namespace AST

// etc.

namespace AST {

struct Block;

// Handles

typedef std::shared_ptr<DecT> DecHandle;

typedef std::shared_ptr<Dec::Var> DecVarHandle;
typedef std::shared_ptr<Dec::Fn> DecFnHandle;
typedef std::shared_ptr<Dec::Prototype> PrototypeHandle;

typedef std::shared_ptr<ExprT> ExprHandle;

typedef std::shared_ptr<StmtT> StmtHandle;
typedef std::shared_ptr<AST::Stmt::Block> StmtBlockHandle;
typedef std::shared_ptr<AST::Stmt::Declaration> StmtDeclarationHandle;

// Etc

typedef std::vector<std::pair<std::string, TypHandle>> ParamVec;
typedef std::vector<std::variant<StmtHandle, DecHandle>> BlockVec;

} // namespace AST
