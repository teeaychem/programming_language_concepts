(* File Fun/Fun.fs
   A strict functional language with integers and first-order
   one-argument functions * sestoft@itu.dk

   Does not support mutually recursive function bindings.

   Performs tail recursion in constant space (because F# does).
*)

module Fun

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
    | Closure of string * string list * expr * value env (* (f, x, fBody, fDeclEnv) *)
    | TupV of expr list

let rec eval (e: expr) (env: value env) : int =
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
        | "&&" -> if i1 = 0 then 0 else i2
        | "||" -> if i1 <> 0 then 1 else i2
        | _ -> failwith ("unknown primitive " + ope)
    | Let(x, eRhs, letBody) ->
        let bodyEnv =
            match eRhs with
            | Tup t -> (x, TupV t) :: env
            | _ -> (x, Int(eval eRhs env)) :: env

        eval letBody bodyEnv
    | If(e1, e2, e3) ->
        let b = eval e1 env
        if b <> 0 then eval e2 env else eval e3 env
    | Letfun(f, xl, fBody, letBody) ->
        let bodyEnv = (f, Closure(f, xl, fBody, env)) :: env
        eval letBody bodyEnv
    | Call(Var f, eArgs) ->
        let fClosure = lookup env f

        match fClosure with
        | Closure(f, x, fBody, fDeclEnv) ->
            let fBodyEnv =
                List.fold2
                    (fun acc nv na ->
                        match na with
                        | Tup t -> (nv, TupV t) :: acc
                        | _ -> (nv, Int(eval na env)) :: acc)
                    ((f, fClosure) :: fDeclEnv)
                    x
                    eArgs

            eval fBody fBodyEnv
        | _ -> failwith "eval Call: not a function"
    | Call _ -> failwith "eval Call: not first-order function"

    | Sel(i, t) ->
        match t with
        | Var t ->
            let t = lookup env t

            match t with
            | TupV t -> eval (t.Item(i - 1)) env
            | _ -> failwith "Selection on a variable which does not refer to a tuple"
        | Tup t -> eval (t.Item(i - 1)) env
        | _ -> failwith "Selection on something other than a variable"



    | Tup _ -> failwith "eval Tup"
    | Fun(_, _) -> failwith "Not Implemented"

(* Evaluate in empty environment: program must have no free variables: *)

let run e = eval e []
