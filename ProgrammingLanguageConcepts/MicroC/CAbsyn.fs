(* File MicroC/Absyn.fs
   Abstract syntax of micro-C, an imperative language.
   sestoft@itu.dk 2009-09-25

   Must precede Interp.fs, Comp.fs and Contcomp.fs in Solution Explorer
 *)

module CAbsyn

type typ =
    | TypA of typ * int option // Array type
    | TypC // Type char
    | TypI // Type int
    | TypP of typ // Pointer type

and expr =
    | Access of access // x    or  *p    or  a[e]
    | Addr of access // &x   or  &*p   or  &a[e]
    // | Andalso of expr * expr // Sequential and
    | Assign of access * expr // x=e  or  *p=e  or  a[e]=e
    | Call of string * expr list // Function call f(...)
    | CstI of int // Constant
    | Ite of expr * expr * expr
    // | Orelse of expr * expr // Sequential or
    | PreDec of access // --i or --a[e]
    | PreInc of access // ++i or ++a[e]
    | Prim1 of string * expr // Unary primitive operator
    | Prim2 of string * expr * expr // Binary primitive operator

and access =
    | AccDeref of expr // Pointer dereferencing *p
    | AccIndex of access * expr // Array indexing a[e]
    | AccVar of string // Variable access x

and stmt =
    | Block of stmtordec list // Block: grouping and scope
    | Expr of expr // Expression statement   e;
    | If of expr * stmt * stmt // Conditional
    | Return of expr option // Return from method
    | While of expr * stmt // While loop

and stmtordec =
    | Dec of typ * string // Local variable declaration
    | DecA of typ * string * expr // Local variable declaration and assignment
    | Stmt of stmt // A statement

and topdec =
    | Fundec of typ option * string * (typ * string) list * stmt
    | Vardec of typ * string
    | VardecA of typ * string * expr

and program = Prog of topdec list
