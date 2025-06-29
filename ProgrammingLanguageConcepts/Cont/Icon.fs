(* File Cont/Icon.fs

   Abstract syntax and interpreter for micro-Icon, a language where an
   expression can produce more than one result.

   sestoft@itu.dk * 2010-05-18

   ---

   For a description of micro-Icon, see Chapter 11: Continuations, in
   Programming Language Concepts for Software Developers.

   As described there, the interpreter (eval e cont econt) has two
   continuations:

      * a success continuation cont, that is called on the result v of
        the expression e, in case it has one;

      * a failure continuation econt, that is called on () in case the
        expression e has no result. *)

module Icon

(* Micro-Icon abstract syntax *)

type expr =
    | CstI of int
    | CstS of string
    | FromTo of int * int
    | Write of expr
    | If of expr * expr * expr
    | Prim of string * expr * expr
    | And of expr * expr
    | Or of expr * expr
    | Seq of expr * expr
    | Every of expr
    | Fail

(* Runtime values and runtime continuations *)

type value =
    | Int of int
    | Str of string

type econt = unit -> value

type cont = value -> econt -> value

(* Print to console *)

let write (v: value) (out: string ref) =
    match v with
    | Int i -> out.Value <- out.Value + sprintf "%d " i
    | Str s -> out.Value <- out.Value + sprintf "%s " s

(* Expression evaluation with backtracking *)

let rec eval (e: expr) (cont: cont) (econt: econt) (out: string ref) =
    match e with
    | CstI i -> cont (Int i) econt

    | CstS s -> cont (Str s) econt

    | FromTo(i1, i2) ->
        let rec loop i =
            if i <= i2 then
                cont (Int i) (fun () -> loop (i + 1))
            else
                econt ()

        loop i1

    | Write e ->
        eval
            e
            (fun v ->
                fun econt ->
                    write v out
                    cont v econt)
            econt
            out
    | If(e1, e2, e3) -> eval e1 (fun _ -> fun _ -> eval e2 cont econt out) (fun () -> eval e3 cont econt out) out

    | Prim(ope, e1, e2) ->
        eval
            e1
            (fun v1 ->
                fun econt1 ->
                    eval
                        e2
                        (fun v2 ->
                            fun econt2 ->
                                match ope, v1, v2 with
                                | "+", Int i1, Int i2 -> cont (Int(i1 + i2)) econt2
                                | "*", Int i1, Int i2 -> cont (Int(i1 * i2)) econt2
                                | "<", Int i1, Int i2 ->
                                    match i1 < i2 with
                                    | true -> cont (Int i2) econt2
                                    | false -> econt2 ()
                                | _ -> Str "unknown prim2")
                        econt1
                        out)
            econt
            out
    | And(e1, e2) -> eval e1 (fun _ -> fun econt1 -> eval e2 cont econt1 out) econt out

    | Or(e1, e2) -> eval e1 cont (fun () -> eval e2 cont econt out) out

    | Seq(e1, e2) -> eval e1 (fun _ -> fun _econt1 -> eval e2 cont econt out) (fun () -> eval e2 cont econt out) out
    | Every e -> eval e (fun _ -> fun econt1 -> econt1 ()) (fun () -> cont (Int 0) econt) out

    | Fail -> econt ()


let run e (out: string ref) =
    eval
        e
        (fun v -> fun _ -> v)
        (fun () ->
            out.Value <- out.Value + "Failed"
            Int 0)
        out


(* Examples in abstract syntax *)
