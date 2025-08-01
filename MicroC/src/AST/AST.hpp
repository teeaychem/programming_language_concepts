#pragma once

#include <format>
#include <map>
#include <string>

#include "llvm/IR/DIBuilder.h"
struct LLVMBundle;

// Types

namespace AST {

namespace Typ {

enum class Kind {
  Ptr,  // An array.
  Int,  // An integer.
  Char, // A character.
  Void, // Unspecified, and requires completion to be a usable type.
};

struct Ptr;
struct Int;
struct Char;
struct Void;

} // namespace Typ

struct TypT;
typedef std::shared_ptr<TypT> TypHandle;

struct TypT {
  // Generate the representation of this type.
  virtual llvm::Type *llvm(LLVMBundle &hdl) const = 0;

  // The kind of this type, corresponding to a struct.
  virtual Typ::Kind kind() const = 0;

  // String representation.
  virtual std::string to_string(size_t indent) const = 0;

  // Dereference this type, may panic if dereference is not possible.
  virtual std::shared_ptr<TypT> deref() const = 0;

  // Completes the type, may throw if already complete.
  virtual TypHandle complete_data(TypHandle d_typ) = 0;

  virtual ~TypT() = default;
};

typedef std::shared_ptr<Typ::Ptr> TypPtrHandle;

} // namespace AST

typedef std::map<std::string, AST::TypHandle> Env; // Vars have declared type, fns have return type.

// Nodes

namespace AST {

// Node kinds
enum class Kind {
  Expr,
  Stmt,
  Dec,
};

struct NodeT {
  // The kind of node.
  virtual AST::Kind kind_node() const = 0;

  // Generates LLVM IR and returns the resulting LLVM value.
  virtual llvm::Value *codegen(LLVMBundle &bundle) const = 0;

  // A string representation of the node, indented to `indent`.
  virtual std::string to_string(size_t indent) const = 0;

  virtual ~NodeT() = default;
};
} // namespace AST

// Nodes

// Expressions

namespace AST {

namespace Expr {

// Expression node kinds
enum class Kind {
  Call,
  CstI,
  Index,
  Prim1,
  Prim2,
  Var,
};

// Forward declaration of expression nodes
struct Call;
struct CstI;
struct Index;
struct Prim1;
struct Prim2;
struct Var;

// Permitted unary operations
enum class OpUnary {
  AddressOf,
  Dereference,
  Sub,
  Negation,
};

// Permitted binary operations
enum class OpBinary {
  Assign,
  AssignAdd,
  AssignSub,
  AssignMul,
  AssignDiv,
  AssignMod,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Eq,
  Neq,
  Gt,
  Lt,
  Leq,
  Geq,
  And,
  Or
};

} // namespace Expr

struct ExprT : NodeT {

protected:
  // Each expression node is typed, and should be initialised with its type.
  TypHandle typ{nullptr};

public:
  AST::Kind kind_node() const override { return AST::Kind::Expr; }

  // Returns the type of an expression.
  AST::TypHandle type() const {
    if (!this->typ) {
      throw std::logic_error(std::format("No type for {}", this->to_string(0)));
    }
    return this->typ;
  };

  // LLVM IR codegen which returns true if this expression evaluates to true and false otherwise.
  llvm::Value *codegen_eval_true(LLVMBundle &bundle) const;
  // LLVM IR codegen which returns false if this expression evaluates to true and false otherwise.
  llvm::Value *codegen_eval_false(LLVMBundle &bundle) const;

  bool evals_to(AST::Typ::Kind type) const { return this->typ->kind() == type; };

  virtual Expr::Kind kind() const = 0;
};

// Statements

namespace Stmt {

// Statement node kinds
enum class Kind {
  Block,
  Declaration,
  Expr,
  If,
  Return,
  While
};

// Forward declaration of statement nodes
struct Block;
struct Declaration;
struct Expr;
struct If;
struct Return;
struct While;
} // namespace Stmt

struct StmtT : NodeT {
  AST::Kind kind_node() const override { return AST::Kind::Stmt; }
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
  AST::Kind kind_node() const override { return AST::Kind::Dec; }
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
