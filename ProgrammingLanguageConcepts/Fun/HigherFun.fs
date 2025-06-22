(* A functional language with integers and higher-order functions
   sestoft@itu.dk 2009-09-11

   The language is higher-order because the value of an expression may
   be a function (and therefore a function can be passed as argument
   to another function).

   A function definition can have only one parameter, but a
   multiparameter (curried) function can be defined using nested
   function definitions:

      let f x = let g y = x + y in g end in f 6 7 end
 *)

module HigherFun

open FunAbsyn

(* Environment operations *)

type 'v env = (string * 'v) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: r -> if x = y then v else lookup r x

(* A runtime value is an integer or a function closure *)

type value =
    | Int of int
    | Closure of string * string list * expr * value env // (f, x, fBody, fDeclEnv)
    | Clos of string list * expr * value env // (x,body,declEnv)
    | TupV of expr list

let rec eval (e: expr) (env: value env) : value =
    match e with
    | CstI i -> Int i

    | CstB b -> Int(if b then 1 else 0)

    | Var x -> lookup env x

    | Prim(ope, e1, e2) ->
        let v1 = eval e1 env
        let v2 = eval e2 env

        match ope, v1, v2 with
        | "*", Int i1, Int i2 -> Int(i1 * i2)
        | "+", Int i1, Int i2 -> Int(i1 + i2)
        | "-", Int i1, Int i2 -> Int(i1 - i2)
        | "=", Int i1, Int i2 -> Int(if i1 = i2 then 1 else 0)
        | "<", Int i1, Int i2 -> Int(if i1 < i2 then 1 else 0)
        | _ -> failwith "unknown primitive or wrong type"

    | Let(x, eRhs, letBody) ->
        let xVal = eval eRhs env
        let letEnv = (x, xVal) :: env
        eval letBody letEnv

    | If(e1, e2, e3) ->
        match eval e1 env with
        | Int 0 -> eval e3 env
        | Int _ -> eval e2 env
        | _ -> failwith "eval If"

    | Letfun(f, x, fBody, letBody) ->
        let bodyEnv = (f, Closure(f, x, fBody, env)) :: env
        eval letBody bodyEnv


    | Fun(arg, expr) -> Clos(arg, expr, env)

    | Call(eFun, eArgs) ->
        let fClosure = eval eFun env

        match fClosure with
        | Closure(f, x, fBody, fDeclEnv) ->
            let envAdd = List.fold2 (fun acc x eArg -> (x, eval eArg env) :: acc) [] x eArgs

            let fBodyEnv = envAdd @ (f, fClosure) :: fDeclEnv in
            eval fBody fBodyEnv
        | Clos(x, fBody, fDeclEnv) ->
            let envAdd = List.fold2 (fun acc x eArg -> (x, eval eArg env) :: acc) [] x eArgs

            let fBodyEnv = envAdd @ fDeclEnv in

            eval fBody fBodyEnv

        | _ -> failwith "eval Call: not a function"

    | Sel(i, t) ->
        match t with
        | Var t ->
            match lookup env t with
            | TupV t -> eval (t.Item(i - 1)) env
            | _ -> failwith "Selection on a variable which does not refer to a tuple"
        | Tup t -> eval (t.Item(i - 1)) env
        | _ -> failwith "Selection on something other than a variable"

    | Tup t -> failwith "eval Tup"
