(* File Imp/Naive.fs

   A naive imperative language with for- and while-loops
   sestoft@itu.dk * 2009-11-17
*)

module Naive

(* A naive store is a map from names (strings) to values (ints) *)

type naivestore = Map<string, int>

let emptystore: Map<string, int> = Map.empty

let getSto (store: naivestore) x = store.Item x

let setSto (store: naivestore) (k, v) = store.Add(k, v)


(* Abstract syntax for expressions *)

type expr =
    | CstI of int
    | Var of string
    | Prim of string * expr * expr

let rec eval e (store: naivestore) : int =
    match e with
    | CstI i -> i
    | Var x -> getSto store x
    | Prim(ope, e1, e2) ->
        let i1 = eval e1 store
        let i2 = eval e2 store

        match ope with
        | "*" -> i1 * i2
        | "+" -> i1 + i2
        | "-" -> i1 - i2
        | "==" -> if i1 = i2 then 1 else 0
        | "<" -> if i1 < i2 then 1 else 0
        | _ -> failwith "unknown primitive"

type stmt =
    | Asgn of string * expr
    | If of expr * stmt * stmt
    | Block of stmt list
    | For of string * expr * expr * stmt
    | While of expr * stmt
    | Print of expr

let rec exec stmt (store: naivestore) (out: string ref) : naivestore =
    match stmt with
    | Asgn(x, e) -> setSto store (x, eval e store)

    | If(e1, stmt1, stmt2) ->
        if eval e1 store <> 0 then
            exec stmt1 store out
        else
            exec stmt2 store out

    | Block stmts ->
        let rec loop ss sto =
            match ss with
            | [] -> sto
            | s1 :: sr -> loop sr (exec s1 sto out)

        loop stmts store

    | For(x, estart, estop, stmt) ->
        let start = eval estart store
        let stop = eval estop store

        let rec loop i sto =
            if i > stop then
                sto
            else
                loop (i + 1) (exec stmt (setSto sto (x, i)) out)

        loop start store

    | While(e, stmt) ->
        let rec loop sto =
            if eval e sto = 0 then sto else loop (exec stmt sto out)

        loop store

    | Print e ->
        out.Value <- sprintf "%d\n" (eval e store)
        store

let run stmt =
    let out = ref ""
    let _ = exec stmt emptystore out
    out.Value.Trim()
