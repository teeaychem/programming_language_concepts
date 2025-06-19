(* File TypedFun/TypedFun.fs
   An explicitly typed strict first-order functional language.
   sestoft@itu.dk 2009-09-11

   Different abstract syntax from the first-order and higher-order
   functional language in Fun/Fun.fs and Fun/HigherFun.fs because
   of the explicit types on function parameters and function results.

   Does not admit mutually recursive function bindings.

   Every function takes exactly one argument.

   Type checking.  Explicit types on the argument and result of each
   declared function.  Expressions and variables may have type int or
   bool or a functional type.  Functions are monomorphically and
   explicitly typed.

   There is no lexer or parser specification for this explicitly typed
   language because next week we shall infer types rather than check
   them.
*)

module TypedFun

(* Environment operations *)

type 'v env = (string * 'v) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: r -> if x = y then v else lookup r x

(* A type is int, bool or function *)

type typ =
    | TypI (* int                         *)
    | TypB (* bool                        *)
    | TypF of typ list * typ (* (argumenttype, resulttype)  *)

(* New abstract syntax with explicit types, instead of Absyn.expr: *)

type tyexpr =
    | Call of tyexpr * tyexpr list
    | CstB of bool
    | CstI of int
    | If of tyexpr * tyexpr * tyexpr
    | Let of string * tyexpr * tyexpr
    | Letfun of string * (string * typ) list * tyexpr * typ * tyexpr // (f, (x, xTyp) list , fBody,  rTyp, letBody
    | Prim of string * tyexpr * tyexpr
    | Var of string

(* A runtime value is an integer or a function closure *)

type value =
    | Int of int
    | Closure of string * string list * tyexpr * value env (* (f, x, fBody, fDeclEnv) *)

let rec eval (e: tyexpr) (env: value env) : int =
    match e with
    | CstI i -> i
    | CstB b -> if b then 1 else 0
    | Var x ->
        match lookup env x with
        | Int i -> i
        | _ -> failwith "eval Var"
    | Prim(ope, e1, e2) ->
        let i1 = eval e1 env
        let i2 = eval e2 env

        match ope with
        | "*" -> i1 * i2
        | "+" -> i1 + i2
        | "-" -> i1 - i2
        | "=" -> if i1 = i2 then 1 else 0
        | "<" -> if i1 < i2 then 1 else 0
        | _ -> failwith "unknown primitive"
    | Let(x, eRhs, letBody) ->
        let xVal = Int(eval eRhs env)
        let bodyEnv = (x, xVal) :: env
        eval letBody bodyEnv
    | If(e1, e2, e3) ->
        let b = eval e1 env
        if b <> 0 then eval e2 env else eval e3 env
    | Letfun(f, xTypL, fBody, _, letBody) ->
        let xs = List.map (fun (x, _) -> x) xTypL
        let bodyEnv = (f, Closure(f, xs, fBody, env)) :: env
        eval letBody bodyEnv
    | Call(Var f, eArgs) ->
        let fClosure = lookup env f

        match fClosure with
        | Closure(f, xL, fBody, fDeclEnv) ->

            let xVals = List.map (fun (x, eArg) -> x, Int(eval eArg env)) (List.zip xL eArgs)
            let fBodyEnv = xVals @ (f, fClosure) :: fDeclEnv
            eval fBody fBodyEnv
        | _ -> failwith "eval Call: not a function"
    | Call _ -> failwith "illegal function in Call"

(* Type checking for the first-order functional language: *)

let rec typ (e: tyexpr) (env: typ env) : typ =
    match e with
    | CstI _ -> TypI
    | CstB _ -> TypB
    | Var x -> lookup env x
    | Prim(ope, e1, e2) ->
        let t1 = typ e1 env
        let t2 = typ e2 env

        match ope, t1, t2 with
        | "*", TypI, TypI -> TypI
        | "+", TypI, TypI -> TypI
        | "-", TypI, TypI -> TypI
        | "=", TypI, TypI -> TypB
        | "<", TypI, TypI -> TypB
        | "&", TypB, TypB -> TypB
        | _ -> failwith "unknown op, or type error"
    | Let(x, eRhs, letBody) ->
        let xTyp = typ eRhs env
        let letBodyEnv = (x, xTyp) :: env
        typ letBody letBodyEnv
    | If(e1, e2, e3) ->
        match typ e1 env with
        | TypB ->
            let t2 = typ e2 env
            let t3 = typ e3 env
            if t2 = t3 then t2 else failwith "If: branch types differ"
        | _ -> failwith "If: condition not boolean"
    | Letfun(f, xTypL, fBody, rTyp, letBody) ->
        let argTyps = List.map (fun (_, t) -> t) xTypL
        let fTyp = TypF(argTyps, rTyp)
        let fBodyEnv = xTypL @ (f, fTyp) :: env
        let letBodyEnv = (f, fTyp) :: env

        if typ fBody fBodyEnv = rTyp then
            typ letBody letBodyEnv
        else
            failwith ("Letfun: return type in " + f)
    | Call(Var f, eArgs) ->
        match lookup env f with
        | TypF(xTypsL, rTyp) ->
            if List.forall (fun (eArg, xTyp) -> typ eArg env = xTyp) (List.zip eArgs xTypsL) then
                rTyp
            else
                failwith "Call: wrong argument type(s)"
        | _ -> failwith "Call: unknown function"
    | Call(_, _) -> failwith "Call: illegal function in call"

let typeCheck e = typ e []
