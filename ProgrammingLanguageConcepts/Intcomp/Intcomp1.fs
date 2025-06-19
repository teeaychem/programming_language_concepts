(* Programming language concepts for software developers, 2012-02-17 *)

(* Evaluation, checking, and compilation of object language expressions *)
(* Stack machines for expression evaluation                             *)

(* Object language expressions with variable bindings and nested scope *)

module Intcomp1

type eval_env = (string * int) list

type expr =
    | CstI of int
    | Var of string
    | Let of string * expr * expr
    | Lets of (string * expr) list * expr
    | Prim of string * expr * expr


(* Evaluation of expressions with variables and bindings *)

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: r -> if x = y then v else lookup r x

let rec eval e (env: eval_env) : int =
    match e with
    | CstI i -> i
    | Var x -> lookup env x
    | Let(x_var, x_val, ebody) ->
        let x_env = (x_var, eval x_val env) :: env
        eval ebody x_env

    | Lets(evals, ebody) ->

        let renv = List.fold (fun env (xvar, xval) -> (xvar, eval xval env) :: env) [] evals
        eval ebody renv

    | Prim("+", e1, e2) -> eval e1 env + eval e2 env
    | Prim("*", e1, e2) -> eval e1 env * eval e2 env
    | Prim("-", e1, e2) -> eval e1 env - eval e2 env
    | Prim _ -> failwith "unknown primitive"

let run e = eval e []


(* Closedness *)

// let mem x vs = List.exists (fun y -> x=y) vs

let rec mem x vs =
    match vs with
    | [] -> false
    | v :: vr -> x = v || mem x vr

(* Checking whether an expression is closed.  The vs is a list of the bound variables.  *)

let rec closedin (e: expr) (vs: string list) : bool =
    match e with
    | CstI _ -> true
    | Var x -> List.exists (fun y -> x = y) vs
    | Let(x, erhs, ebody) ->
        let vs1 = x :: vs
        closedin erhs vs && closedin ebody vs1
    | Prim(_, e1, e2) -> closedin e1 vs && closedin e2 vs
    | Lets(_, _) -> failwith "Not Implemented"

(* An expression is closed if it is closed in the empty environment *)

let closed1 e = closedin e []


(* Substitution of expressions for variables *)

(* This version of lookup returns a Var(x) expression if there is no
   pair (x,e) in the list env --- instead of failing with exception: *)

let rec lookOrSelf env x =
    match env with
    | [] -> Var x
    | (y, e) :: r -> if x = y then e else lookOrSelf r x

(* Remove (x, _) from env: *)

let rec remove env x =
    match env with
    | [] -> []
    | (y, e) :: r -> if x = y then r else (y, e) :: remove r x

(* Naive substitution, may capture free variables: *)

let rec nsubst (e: expr) (env: (string * expr) list) : expr =
    match e with
    | CstI _ -> e
    | Var x -> lookOrSelf env x
    | Let(x, erhs, ebody) ->
        let newenv = remove env x
        Let(x, nsubst erhs env, nsubst ebody newenv)
    | Lets(_, _) -> failwith "nsubst is not implemented for Lets"
    | Prim(ope, e1, e2) -> Prim(ope, nsubst e1 env, nsubst e2 env)

let newVar: string -> string =
    let n = ref 0

    let varMaker x =
        n.Value <- 1 + n.Value
        x + string n.Value

    varMaker

(* Correct, capture-avoiding substitution *)

let rec subst (e: expr) (env: (string * expr) list) : expr =
    match e with
    | CstI _ -> e
    | Var x -> lookOrSelf env x
    | Let(x, erhs, ebody) ->
        let newx = newVar x
        let newenv = (x, Var newx) :: remove env x
        Let(newx, subst erhs env, subst ebody newenv)
    | Prim(ope, e1, e2) -> Prim(ope, subst e1 env, subst e2 env)
    | Lets(_, _) -> failwith "Not Implemented"


(* Operations on sets, represented as lists. *)
// Simple but inefficient; one could use binary trees, hashtables or splaytrees for  efficiency.

(* union(xs, ys) is the set of all elements in xs or ys, without duplicates *)

let rec union (xs, ys) =
    match xs with
    | [] -> ys
    | x :: xr -> if mem x ys then union (xr, ys) else x :: union (xr, ys)

(* minus xs ys  is the set of all elements in xs but not in ys *)

let rec minus (xs, ys) =
    match xs with
    | [] -> []
    | x :: xr -> if mem x ys then minus (xr, ys) else x :: minus (xr, ys)

(* Find all variables that occur free in expression e *)

let rec freevars e : string list =
    match e with
    | CstI _ -> []
    | Var x -> [ x ]
    | Let(x, erhs, ebody) -> union (freevars erhs, minus (freevars ebody, [ x ]))
    | Lets(lets, ebody) ->

        let free, bound =
            List.fold
                (fun (free, bound) (xvar, xval) -> union (freevars xval, minus (free, [ xvar ])), xvar :: bound)
                ([], [])
                (List.rev lets)

        union (free, minus (freevars ebody, bound))

    | Prim(_, e1, e2) -> union (freevars e1, freevars e2)

(* Alternative definition of closed *)

let closed2 e = freevars e = []


(* Compilation to target expressions with numerical indexes instead of symbolic variable names.  *)

type texpr = (* target expressions *)
    | TCstI of int
    | TVar of int (* index into runtime environment *)
    | TLet of texpr * texpr (* erhs and ebody *)
    | TLets of (texpr list) * texpr (* erhs and ebody *)
    | TPrim of string * texpr * texpr


(* Map variable name to variable index at compile-time *)

let rec getindex vs x =
    match vs with
    | [] -> failwith "Variable not found"
    | y :: yr -> if x = y then 0 else 1 + getindex yr x

(* Compiling from expr to texpr *)

let rec tcomp (e: expr) (cenv: string list) : texpr =
    match e with
    | CstI i -> TCstI i
    | Var x -> TVar(getindex cenv x)
    | Let(x, erhs, ebody) ->
        let cenv1 = x :: cenv
        TLet(tcomp erhs cenv, tcomp ebody cenv1)

    | Lets(elist, ebody) ->

        let rvals, renv =
            List.fold (fun (avals, aenv) (a, b) -> tcomp b aenv :: avals, a :: aenv) ([], []) elist

        TLets(List.rev rvals, tcomp ebody renv)


    | Prim(ope, e1, e2) -> TPrim(ope, tcomp e1 cenv, tcomp e2 cenv)

(* Evaluation of target expressions with variable indexes. *)
// The run-time environment renv is a list of variable values (ints).

let rec teval (e: texpr) (renv: int list) : int =
    match e with
    | TCstI i -> i
    | TVar n -> List.item n renv
    | TLet(erhs, ebody) ->
        let xval = teval erhs renv
        let renv1 = xval :: renv
        teval ebody renv1
    | TLets(elist, ebody) ->

        let renvx = List.fold (fun aenv elem -> teval elem aenv :: aenv) [] elist
        teval ebody renvx

    | TPrim("+", e1, e2) -> teval e1 renv + teval e2 renv
    | TPrim("*", e1, e2) -> teval e1 renv * teval e2 renv
    | TPrim("-", e1, e2) -> teval e1 renv - teval e2 renv
    | TPrim _ -> failwith "unknown primitive"

(* Correctness: eval e []  equals  teval (tcomp e []) [] *)


(* Stack machines *)

(* Stack machine instructions. *)
// An expressions in postfix or reverse Polish form is a list of stack machine instructions.

type rinstr =
    | RCstI of int
    | RAdd
    | RSub
    | RMul
    | RDup
    | RSwap

(* A simple stack machine for evaluation of variable-free expressions in postfix form *)

let rec reval (inss: rinstr list) (stack: int list) : int =
    match inss, stack with
    | [], v :: _ -> v
    | [], [] -> failwith "reval: no result on stack!"
    | RCstI i :: insr, stk -> reval insr (i :: stk)
    | RAdd :: insr, i2 :: i1 :: stkr -> reval insr (i1 + i2 :: stkr)
    | RSub :: insr, i2 :: i1 :: stkr -> reval insr (i1 - i2 :: stkr)
    | RMul :: insr, i2 :: i1 :: stkr -> reval insr (i1 * i2 :: stkr)
    | RDup :: insr, i1 :: stkr -> reval insr (i1 :: i1 :: stkr)
    | RSwap :: insr, i2 :: i1 :: stkr -> reval insr (i1 :: i2 :: stkr)
    | _ -> failwith "reval: too few operands on stack"


(* Compilation of a variable-free expression to a rinstr list *)

let rec rcomp (e: expr) : rinstr list =
    match e with
    | CstI i -> [ RCstI i ]
    | Var _ -> failwith "rcomp cannot compile Var"
    | Let _ -> failwith "rcomp cannot compile Let"
    | Lets _ -> failwith "rcomp cannot compile Lets"
    | Prim("+", e1, e2) -> rcomp e1 @ rcomp e2 @ [ RAdd ]
    | Prim("*", e1, e2) -> rcomp e1 @ rcomp e2 @ [ RMul ]
    | Prim("-", e1, e2) -> rcomp e1 @ rcomp e2 @ [ RSub ]
    | Prim _ -> failwith "unknown primitive"

(* Correctness: eval e []  equals  reval (rcomp e) [] *)


(* Storing intermediate results and variable bindings in the same stack *)

type sinstr =
    | SCstI of int (* push integer           *)
    | SVar of int (* push variable from env *)
    | SAdd (* pop args, push sum     *)
    | SSub (* pop args, push diff.   *)
    | SMul (* pop args, push product *)
    | SPop (* pop value/unbind var   *)
    | SSwap (* exchange top and next  *)


let rec seval (inss: sinstr list) (stack: int list) =
    match inss, stack with
    | [], v :: _ -> v
    | [], [] -> failwith "seval: no result on stack"
    | SCstI i :: insr, stk -> seval insr (i :: stk)
    | SVar i :: insr, stk -> seval insr (List.item i stk :: stk)
    | SAdd :: insr, i2 :: i1 :: stkr -> seval insr (i1 + i2 :: stkr)
    | SSub :: insr, i2 :: i1 :: stkr -> seval insr (i1 - i2 :: stkr)
    | SMul :: insr, i2 :: i1 :: stkr -> seval insr (i1 * i2 :: stkr)
    | SPop :: insr, _ :: stkr -> seval insr stkr
    | SSwap :: insr, i2 :: i1 :: stkr -> seval insr (i1 :: i2 :: stkr)
    | _ -> failwith "seval: too few operands on stack"


(* A compile-time variable environment representing the state of the run-time stack. *)

type stackvalue =
    | Value (* A computed value *)
    | Bound of string (* A bound variable *)

(* Compilation to a list of instructions for a unified-stack machine *)

let rec scomp (e: expr) (cenv: stackvalue list) : sinstr list =
    match e with
    | CstI i -> [ SCstI i ]
    | Var x -> [ SVar(getindex cenv (Bound x)) ]
    | Let(x, erhs, ebody) -> scomp erhs cenv @ scomp ebody (Bound x :: cenv) @ [ SSwap; SPop ]
    | Prim("+", e1, e2) -> scomp e1 cenv @ scomp e2 (Value :: cenv) @ [ SAdd ]
    | Prim("-", e1, e2) -> scomp e1 cenv @ scomp e2 (Value :: cenv) @ [ SSub ]
    | Prim("*", e1, e2) -> scomp e1 cenv @ scomp e2 (Value :: cenv) @ [ SMul ]
    | Prim _ -> failwith "scomp: unknown operator"
    | Lets(_, _) -> failwith "Not Implemented"



(* Output the integers in list inss to the text file called fname: *)

let intsToFile (inss: int list) (fname: string) : string =
    let text = String.concat " " (List.map string inss)

    if not (System.IO.File.Exists fname) then

        let creation_code = System.IO.File.Create fname
        creation_code.Close()

    System.IO.File.WriteAllText(fname, text)

    System.IO.Path.GetFullPath fname

(* -----------------------------------------------------------------  *)

let rec assemble (instructions: sinstr list) : int list =
    match instructions with
    | i :: is ->
        let alist = assemble is

        match i with
        | SCstI cst -> 0 :: cst :: alist
        | SVar var -> 1 :: var :: alist
        | SAdd -> 2 :: alist
        | SSub -> 3 :: alist
        | SMul -> 4 :: alist
        | SPop -> 5 :: alist
        | SSwap -> 6 :: alist

    | [] -> []
