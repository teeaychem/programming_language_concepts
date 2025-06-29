(* File Cont/Contfun.fs

   A strict functional language with integers, first-order functions,
   and exceptions * sestoft@itu.dk 2009-09-24

   Exceptions in the functional object language are modelled without
   using exceptions in the meta-language.  Instead the interpreter
   uses continuations to represent the rest of the computation.
   The interpreters are written in continuation-passing style, and
   implement tail calls in constant space.

   This file contains two interpreters:

     * coEval1 allows exceptions to be thrown, but not caught; this
       is implemented using a single continuation (which is used by
       ordinary computations but ignored when an exception is thrown)

     * coEval2 allows exceptions to be thrown and caught; this is
       implemented using two continuations: the success continuation
       is used by ordinary computations, and the error continuation
       is used when an exception is thrown. *)

module Contfun

(* Simple environment operations *)

type 'data env = (string * 'data) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: yr -> if x = y then v else lookup yr x

(* Abstract syntax of functional language with exceptions *)

type exn = Exn of string

type expr =
    | CstI of int
    | CstB of bool
    | Var of string
    | Let of string * expr * expr
    | Prim of string * expr * expr
    | If of expr * expr * expr
    | Letfun of string * string * expr * expr (* (f, x, fbody, ebody) *)
    | Call of string * expr
    | Raise of exn
    | TryWith of expr * exn * expr (* try e1 with exn -> e2 *)

type value =
    | Int of int
    | Closure of string * string * expr * value env (* (f, x, fBody, fDeclEnv) *)

type answer =
    | Result of int
    | Abort of string

(* This interpreter coEval1 takes the following arguments:

    * An expression e to evaluate.
    * An environment env in which to evalute it.
    * A success continuation cont which accepts as argument the value
      of the expression.

   It returns an answer: Result i or Abort s.  When the evaluation
   of e succeeds, it applies the success continuation to its result,
   and when e raises an exception (Exn s), it returns Abort s.
   Since there is no error continuation, there is no provision for
   handling raised exceptions. *)

let rec coEval1 (e: expr) (env: value env) (cont: int -> answer) : answer =
    match e with
    | CstI i -> cont i
    | CstB b -> cont (if b then 1 else 0)
    | Var x ->
        match lookup env x with
        | Int i -> cont i
        | _ -> Abort "coEval1 Var"
    | Prim(ope, e1, e2) ->
        coEval1 e1 env (fun i1 ->
            coEval1 e2 env (fun i2 ->
                match ope with
                | "*" -> cont (i1 * i2)
                | "+" -> cont (i1 + i2)
                | "-" -> cont (i1 - i2)
                | "=" -> cont (if i1 = i2 then 1 else 0)
                | "<" -> cont (if i1 < i2 then 1 else 0)
                | _ -> Abort "unknown primitive"))
    | Let(x, eRhs, letBody) ->
        coEval1 eRhs env (fun xVal ->
            let bodyEnv = (x, Int xVal) :: env
            coEval1 letBody bodyEnv cont)
    | If(e1, e2, e3) -> coEval1 e1 env (fun b -> if b <> 0 then coEval1 e2 env cont else coEval1 e3 env cont)
    | Letfun(f, x, fBody, letBody) ->
        let bodyEnv = (f, Closure(f, x, fBody, env)) :: env
        coEval1 letBody bodyEnv cont
    | Call(f, eArg) ->
        let fClosure = lookup env f

        match fClosure with
        | Closure(f, x, fBody, fDeclEnv) ->
            coEval1 eArg env (fun xVal ->
                let fBodyEnv = (x, Int xVal) :: (f, fClosure) :: fDeclEnv
                coEval1 fBody fBodyEnv cont)
        | _ -> Abort "eval Call: not a function"
    | Raise(Exn s) -> Abort s
    | TryWith(_e1, _exn, _e2) -> Abort "Not implemented"


let eval1 e env = coEval1 e env (fun v -> Result v)
let run1 e = eval1 e []


(* This interpreter coEval2 takes the following arguments:

    * An expression e to evaluate.
    * An environment env in which to evalute it.
    * A success continuation cont which accepts as argument the value
      of the expression.
    * A error continuation econt, which is applied when an exception
      is thrown

   It returns an answer: Result i or Abort s.  When the evaluation
   of e succeeds, it applies the success continuation to its result,
   and when e raises an exception exn, it applies the failure
   continuation to exn.  The failure continuation may choose to catch
   the exception. *)

let rec coEval2 (e: expr) (env: value env) (cont: int -> answer) (econt: exn -> answer) : answer =
    match e with
    | CstI i -> cont i
    | CstB b -> cont (if b then 1 else 0)
    | Var x ->
        match lookup env x with
        | Int i -> cont i
        | _ -> Abort "coEval2 Var"
    | Prim(ope, e1, e2) ->
        coEval2
            e1
            env
            (fun i1 ->
                coEval2
                    e2
                    env
                    (fun i2 ->
                        match ope with
                        | "*" -> cont (i1 * i2)
                        | "+" -> cont (i1 + i2)
                        | "-" -> cont (i1 - i2)
                        | "=" -> cont (if i1 = i2 then 1 else 0)
                        | "<" -> cont (if i1 < i2 then 1 else 0)
                        | _ -> Abort "unknown primitive")
                    econt)
            econt
    | Let(x, eRhs, letBody) ->
        coEval2
            eRhs
            env
            (fun xVal ->
                let bodyEnv = (x, Int xVal) :: env
                coEval2 letBody bodyEnv cont econt)
            econt
    | If(e1, e2, e3) ->
        coEval2
            e1
            env
            (fun b ->
                if b <> 0 then
                    coEval2 e2 env cont econt
                else
                    coEval2 e3 env cont econt)
            econt
    | Letfun(f, x, fBody, letBody) ->
        let bodyEnv = (f, Closure(f, x, fBody, env)) :: env
        coEval2 letBody bodyEnv cont econt
    | Call(f, eArg) ->
        let fClosure = lookup env f

        match fClosure with
        | Closure(f, x, fBody, fDeclEnv) ->
            coEval2
                eArg
                env
                (fun xVal ->
                    let fBodyEnv = (x, Int xVal) :: (f, fClosure) :: fDeclEnv
                    coEval2 fBody fBodyEnv cont econt)
                econt
        | _ -> raise (Failure "eval Call: not a function")
    | Raise exn -> econt exn
    | TryWith(e1, exn, e2) ->
        let econt1 thrown =
            if thrown = exn then
                coEval2 e2 env cont econt
            else
                econt thrown

        coEval2 e1 env cont econt1

(* The top-level error continuation returns the continuation, adding the text Uncaught exception *)

let eval2 e env =
    coEval2 e env (fun v -> Result v) (fun (Exn s) -> Abort("Uncaught exception: " + s))

let run2 e = eval2 e []
