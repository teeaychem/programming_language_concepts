(* Fun/Absyn.fs * Abstract syntax for micro-ML, a functional language *)

module FunAbsyn

type expr =
    | Call of expr * expr list
    | CstB of bool
    | CstI of int
    | If of expr * expr * expr
    | Let of string * expr * expr
    | Letfun of string * string list * expr * expr (* (f, x, fBody, letBody) *)
    | Prim of string * expr * expr
    | Sel of int * expr
    | Tup of expr list
    | Var of string
