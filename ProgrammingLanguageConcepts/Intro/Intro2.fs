(* Programming language concepts for software developers, 2010-08-28 *)

(* Evaluating simple expressions with variables *)

module Intro2

(* Association lists map object language variables to their values *)

type env = (string * int) list

let rec lookup (env: env) ref =
    match env with
    | [] -> failwith (sprintf "%s not found" ref)
    | (k, v) :: env -> if ref = k then v else lookup env ref


(* Object language expressions with variables *)

type expr =
    | CstI of int
    | Var of string
    | Prim of string * expr * expr
    | If of expr * expr * expr


(* Evaluation within an environment *)

let rec eval_basic e (env: env) : int =
    match e with
    | CstI i -> i
    | Var x -> lookup env x
    | Prim("+", e1, e2) -> eval_basic e1 env + eval_basic e2 env
    | Prim("*", e1, e2) -> eval_basic e1 env * eval_basic e2 env
    | Prim("-", e1, e2) -> eval_basic e1 env - eval_basic e2 env
    | Prim("max", e1, e2) -> max (eval_basic e1 env) (eval_basic e2 env)
    | Prim("min", e1, e2) -> min (eval_basic e1 env) (eval_basic e2 env)
    | Prim("==", e1, e2) -> if eval_basic e1 env <> eval_basic e2 env then 0 else 1
    | Prim _ -> failwith "unknown primitive"
    | _ -> failwith "uncovered expr"


let rec eval e (env: env) : int =
    match e with
    | CstI i -> i
    | Var x -> lookup env x
    | Prim(op, e1, e2) ->
        let e1 = eval e1 env
        let e2 = eval e2 env

        match op with
        | "+" -> e1 + e2
        | "*" -> e1 * e2
        | "-" -> e1 - e2
        | "max" -> max e1 e2
        | "min" -> min e1 e2
        | "==" -> if e1 <> e2 then 0 else 1
        | "**" -> if e1 <> e2 then 0 else 1
        | _ -> failwith (sprintf "unknown operator '{%s}'" op)
    | If(c, a, b) -> if eval c env = 1 then eval a env else eval b env


module AwL =

    type aexpr =
        | CstI of int
        | Var of string
        | Add of aexpr * aexpr
        | Mul of aexpr * aexpr
        | Sub of aexpr * aexpr
        | Pow of aexpr * aexpr


    let rec eval_aexpr e (env: env) : int =
        match e with
        | CstI i -> i
        | Var x -> lookup env x
        | Add(a, b) -> eval_aexpr a env + eval_aexpr b env
        | Mul(a, b) -> eval_aexpr a env * eval_aexpr b env
        | Sub(a, b) -> eval_aexpr a env - eval_aexpr b env
        | Pow(a, b) -> pown (eval_aexpr a env) (eval_aexpr b env)

    let rec fmt (e: aexpr) : string =
        match e with
        | CstI i -> sprintf "%d" i
        | Var x -> sprintf "%s" x
        | Add(a, b) -> sprintf "(%s + %s)" (fmt a) (fmt b)
        | Mul(a, b) -> sprintf "(%s * %s)" (fmt a) (fmt b)
        | Sub(a, b) -> sprintf "(%s - %s)" (fmt a) (fmt b)
        | Pow(a, b) -> sprintf "(%s ** %s)" (fmt a) (fmt b)

    let rec simplify e : aexpr =
        let emptyenv: env = []

        match e with
        | CstI _
        | Var _ -> e

        | Add(CstI 0, f)
        | Add(f, CstI 0) -> simplify f
        | Add(CstI _, CstI _) -> CstI(eval_aexpr e emptyenv)
        | Add(CstI a, Add(b, CstI c)) -> Add(simplify (Add(CstI a, CstI c)), b)
        | Add(CstI a, Add(CstI b, c)) -> Add(simplify (Add(CstI a, CstI b)), c)
        | Add(Add(Var v, b), c) -> Add(Var v, simplify (Add(simplify b, simplify c)))
        | Add(Add(a, Var v), c) -> Add(Var v, simplify (Add(simplify a, simplify c)))
        | Add(a, b) ->
            let intermediate = Add(simplify a, simplify b)
            if intermediate <> e then simplify intermediate else e

        | Mul(CstI 0, _)
        | Mul(_, CstI 0) -> CstI 0
        | Mul(CstI 1, f)
        | Mul(f, CstI 1) -> simplify f
        | Mul(CstI _, CstI _) -> CstI(eval_aexpr e emptyenv)
        | Mul(CstI a, Mul(b, CstI c)) -> Mul(simplify (Mul(CstI a, CstI c)), b)
        | Mul(CstI a, Mul(CstI b, c)) -> Mul(simplify (Mul(CstI a, CstI b)), c)

        | Mul(Mul(Var v, b), c) -> Mul(Var v, simplify (Mul(simplify b, simplify c)))
        | Mul(Mul(a, Var v), c) -> Mul(Var v, simplify (Mul(simplify a, simplify c)))
        | Mul(a, b) ->
            let intermediate = Mul(simplify a, simplify b)
            if intermediate <> e then simplify intermediate else e

        | Sub(f, CstI 0) -> simplify f
        | Sub(CstI _, CstI _) -> CstI(eval_aexpr e emptyenv)
        | Sub(a, b) -> Sub(simplify a, simplify b)

        | Pow(_, CstI 0) -> CstI 1
        | Pow(a, b) -> Pow(simplify a, simplify b)


    let rec diff e (x: string) : aexpr =
        match e with
        | CstI _ -> CstI 0

        | Add(a, b) ->
            let da = diff a x
            let db = diff b x
            Add(da, db)

        | Mul(a, b) ->
            let da = diff a x
            let db = diff b x
            Add(Mul(da, b), Mul(a, db))

        | Pow(Var x, b) -> Mul(b, Pow(Var x, Sub(b, CstI 1)))

        | _ -> e
