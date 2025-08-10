#pragma once

#include <map>
#include <string>

#include "llvm/IR/DIBuilder.h"

// A general header containing forward declarations for AST nodes, types, and (virtual) base structures.
// Also, some useful typedefs and related things.
//
// See the `Node` subfolder for headers containing specific node declarations.

// Used for codegen
struct LLVMBundle;

// Types

namespace AST {

namespace Typ {

enum class Kind {
  Bool, // A boolean.
  Char, // A character.
  Int,  // An integer.
  Ptr,  // An array.
  Void, // Unspecified, and requires completion to be a usable type.
};

struct Bool;
struct Char;
struct Int;
struct Ptr;
struct Void;

} // namespace Typ

struct TypT;
typedef std::shared_ptr<TypT> TypHandle;

struct TypT {
  // Generate the representation of this type.
  virtual llvm::Type *codegen(LLVMBundle &bundle) const = 0;

  // The kind of this type, corresponding to a struct.
  virtual Typ::Kind kind() const = 0;

  // Whether the kind is of `kind`.
  bool is_kind(AST::Typ::Kind kind) { return this->kind() == kind; };

  // String representation.
  virtual std::string to_string(size_t indent = 0) const = 0;

  // Dereference this type, may panic if dereference is not possible.
  virtual std::shared_ptr<TypT> deref() const = 0;

  // Completes the type, may throw if already complete.
  virtual TypHandle complete_with(TypHandle d_typ) = 0;

  virtual llvm::Constant *defaultgen(LLVMBundle &bundle) const {
    return llvm::Constant::getNullValue(this->codegen(bundle));
  };

  virtual ~TypT() = default;
};

typedef std::shared_ptr<Typ::Ptr> TypPtrHandle;

} // namespace AST

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
  virtual std::string to_string(size_t indent = 0) const = 0;

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
  Cast,
  CstI,
  Index,
  Prim1,
  Prim2,
  Var,
};

// Forward declaration of expression nodes
struct Call;
struct Cast;
struct CstI;
struct Index;
struct Prim1;
struct Prim2;
struct Var;

typedef std::shared_ptr<Call> CallHandle;
typedef std::shared_ptr<Cast> CastHandle;
typedef std::shared_ptr<CstI> CstIHandle;
typedef std::shared_ptr<Index> IndexHandle;
typedef std::shared_ptr<Prim1> Prim1Handle;
typedef std::shared_ptr<Prim2> Prim2Handle;
typedef std::shared_ptr<Var> VarHandle;

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
  AST::TypHandle type() const { return this->typ; };

  // The kind of type `this` has.
  AST::Typ::Kind type_kind() const { return this->type()->kind(); }

  // LLVM IR codegen which returns true if this expression does not have (LLVM IR) null value for its type and false otherwise.
  llvm::Value *codegen_eval_true(LLVMBundle &bundle) const;

  // LLVM IR codegen which returns true if this expression does not evaluate to true and false otherwise.
  llvm::Value *codegen_eval_false(LLVMBundle &bundle) const;

  // Returns true if `this` has type of `kind`.
  bool typ_has_kind(AST::Typ::Kind kind) const { return this->typ->kind() == kind; };

  virtual Expr::Kind kind() const = 0;
};

typedef std::shared_ptr<ExprT> ExprHandle;

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

typedef std::shared_ptr<Block> BlockHandle;
typedef std::shared_ptr<Declaration> DeclarationHandle;
typedef std::shared_ptr<Expr> ExprHandle;
typedef std::shared_ptr<If> IfHandle;
typedef std::shared_ptr<Return> ReturnHandle;
typedef std::shared_ptr<While> WhileHandle;

} // namespace Stmt

struct StmtT : NodeT {
  AST::Kind kind_node() const override { return AST::Kind::Stmt; }

  // The kind of statement `this` is.
  virtual Stmt::Kind kind() const = 0;

  // True if control of the statement is released through a `return` statement.
  virtual bool returns() const = 0;

  // How many times control may be diverted by a return.
  // E.g. by the `then` branch of an if.
  virtual size_t early_returns() const = 0;

  // How many times control may avoid being diverted.
  // E.g. by the absence of an `else` branch of an if.
  virtual size_t pass_throughs() const = 0;
};

typedef std::shared_ptr<StmtT> StmtHandle;

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

typedef std::shared_ptr<Var> VarHandle;
typedef std::shared_ptr<Fn> FnHandle;
typedef std::shared_ptr<Prototype> PrototypeHandle;

} // namespace Dec

struct DecT : NodeT {
  AST::Kind kind_node() const override { return AST::Kind::Dec; }

  // The kind of declaration `this` is.
  virtual Dec::Kind kind() const = 0;

  // The type of variable declared.
  virtual AST::TypHandle type() const = 0;

  // The variable declared.
  virtual std::string name() const = 0;
};

typedef std::shared_ptr<DecT> DecHandle;
} // namespace AST

// etc.

namespace AST {

// Etc

struct VarTyp {
  // The var.
  std::string var;

  // The type.
  TypHandle typ;

  VarTyp(std::string var, TypHandle typ) : var(var), typ(typ) {};

  // To appease bison, should not be used directly.
  VarTyp() : var("!"), typ(nullptr) {};
};

typedef std::vector<AST::VarTyp> VarTypVec;

typedef std::map<std::string, AST::TypHandle> VarTypMap; // Vars have declared type, fns have return type.

// The (contextual) environment when generating an AST.
// 'Contextual', here, means the env is mutated with relevant declarations.
// And, in particular, it is up to the the mutator to restore any temporary mutations (i.e. local declarations)
struct EnvAST {

  // Function declarations.
  std::map<std::string, AST::Dec::PrototypeHandle> fns{};

  // Variables in scope.
  VarTypMap vars{};

  // String representation of the env.
  std::string to_string();
};

} // namespace AST
