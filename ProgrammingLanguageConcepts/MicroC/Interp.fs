(* File MicroC/Interp.c
   Interpreter for micro-C, a fraction of the C language
   sestoft@itu.dk * 2010-01-07, 2014-10-18

   A value is an integer; it may represent an integer or a pointer,
   where a pointer is just an address in the store (of a variable or
   pointer or the base address of an array).  The environment maps a
   variable to an address (location), and the store maps a location to
   an integer.  This freely permits pointer arithmetics, as in real C.
   Expressions can have side effects.  A function takes a list of
   typed arguments and may optionally return a result.

   For now, arrays can be one-dimensional only.  For simplicity, we
   represent an array as a variable which holds the address of the
   first array element.  This is consistent with the way array-type
   parameters are handled in C (and the way that array-type variables
   were handled in the B language), but not with the way array-type
   variables are handled in C.

   The store behaves as a stack, so all data are stack allocated:
   variables, function parameters and arrays.

   The return statement is not implemented (for simplicity), so all
   functions should have return type void.  But there is as yet no
   typecheck, so be careful.
 *)

module Interp

open CAbsyn

(* ------------------------------------------------------------------- *)

(* Simple environment operations *)

type 'data env = (string * 'data) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: yr -> if x = y then v else lookup yr x

(* A local variable environment also knows the next unused store location *)

type locEnv = int env * int

(* A function environment maps a function name to parameter list and body *)

type paramdecs = (typ * string) list
type funEnv = (paramdecs * stmt) env

(* A global environment consists of a global variable environment and a global function environment *)

type gloEnv = int env * funEnv

(* The store maps addresses (ints) to values (ints): *)

type address = int

type store = Map<address, int>

let emptyStore = Map.empty<address, int>

let setSto (store: store) addr value = store.Add(addr, value)

let getSto (store: store) addr =
    if store.ContainsKey addr then
        store.Item addr
    else
        failwith (sprintf "Store does not contain: `%A`" addr)



let rec initSto loc n store =
    if n = 0 then
        store
    else
        initSto (loc + 1) (n - 1) (setSto store loc -999)

(* Combined environment and store operations *)

(* Extend local variable environment so it maps x to nextloc (the next store location) and set store[nextloc] = v. *)

let bindVar x v (env, nextloc) store : locEnv * store =
    let env1 = (x, nextloc) :: env
    (env1, nextloc + 1), setSto store nextloc v

let rec bindVars xs vs locEnv store : locEnv * store =
    match xs, vs with
    | [], [] -> locEnv, store
    | x1 :: xr, v1 :: vr ->
        let locEnv1, sto1 = bindVar x1 v1 locEnv store
        bindVars xr vr locEnv1 sto1
    | _ -> failwith (sprintf "parameter/argument mismatch %A %A" xs vs)

(* Allocate variable (int or pointer or array): extend environment so
   that it maps variable to next available store location, and initialize store location(s). *)

let rec allocate (typ, x) (env0, nextloc) sto0 : locEnv * store =
    let nextloc1, v, sto1 =
        match typ with
        | TypA(_, Some i) -> nextloc + i, nextloc, initSto nextloc i sto0
        | _ -> nextloc, -1, sto0

    bindVar x v (env0, nextloc1) sto1

(* Build global environment of variables and functions.
    For global variables, store locations are reserved;
    for global functions, just add to global function environment. *)

let initEnvAndStore (topdecs: topdec list) : locEnv * funEnv * store =
    let rec addv decs locEnv funEnv store =
        match decs with
        | [] -> locEnv, funEnv, store
        | Vardec(typ, x) :: decr ->
            let locEnv1, sto1 = allocate (typ, x) locEnv store
            addv decr locEnv1 funEnv sto1
        | VardecA _ :: _ -> failwith "Gloabl initialisation is only supported when compiling"

        | Fundec(_, f, xs, body) :: decr -> addv decr locEnv ((f, (xs, body)) :: funEnv) store

    addv topdecs ([], 0) [] emptyStore

(* ------------------------------------------------------------------- *)

(* Interpreting micro-C statements *)

let rec exec stmt (locEnv: locEnv) (gloEnv: gloEnv) (store: store) (out: string ref) : store =
    match stmt with
    | If(e, stmt1, stmt2) ->
        let v, store1 = eval e locEnv gloEnv store out

        if v <> 0 then
            exec stmt1 locEnv gloEnv store1 out
        else
            exec stmt2 locEnv gloEnv store1 out
    | While(e, body) ->
        let rec loop store1 =
            let v, store2 = eval e locEnv gloEnv store1 out

            if v <> 0 then
                loop (exec body locEnv gloEnv store2 out)
            else
                store2

        loop store
    | Expr e ->
        let _, store1 = eval e locEnv gloEnv store out
        store1
    | Block stmts ->
        let rec loop ss (locEnv, store) =
            match ss with
            | [] -> store
            | s1 :: sr -> loop sr (stmtordec s1 locEnv gloEnv store out)

        loop stmts (locEnv, store)
    | Return _ -> failwith "return not implemented"

and stmtordec stmtordec locEnv gloEnv store out =
    match stmtordec with
    | Stmt stmt -> locEnv, exec stmt locEnv gloEnv store out

    | Dec(typ, x) -> allocate (typ, x) locEnv store

    | DecA(typ, var, expr) ->

        let locEnv, store = allocate (typ, var) locEnv store
        let res, store = eval expr locEnv gloEnv store out
        let loc, store = access (AccVar var) locEnv gloEnv store out
        let _, store = res, setSto store loc res
        locEnv, store


(* Evaluating micro-C expressions *)

and eval e locEnv gloEnv store out : int * store =
    match e with
    | Access acc ->
        let loc, store1 = access acc locEnv gloEnv store out
        getSto store1 loc, store1

    | Assign(acc, e) ->
        let loc, store1 = access acc locEnv gloEnv store out
        let res, store2 = eval e locEnv gloEnv store1 out
        res, setSto store2 loc res

    | CstI i -> i, store

    | Addr acc -> access acc locEnv gloEnv store out

    | Prim1(ope, e1) ->
        let i1, store1 = eval e1 locEnv gloEnv store out

        let res =
            match ope with
            | "!" -> if i1 = 0 then 1 else 0
            | "printi" ->
                out.Value <- out.Value + sprintf "%d " i1
                i1
            | "printc" ->
                out.Value <- out.Value + sprintf "%c " (char i1)
                i1
            | _ -> failwith ("unknown primitive " + ope)

        res, store1

    | Prim2(ope, e1, e2) ->
        let i1, store1 = eval e1 locEnv gloEnv store out
        let i2, store2 = eval e2 locEnv gloEnv store1 out

        let res =
            match ope with
            | "*" -> i1 * i2
            | "+" -> i1 + i2
            | "-" -> i1 - i2
            | "/" -> i1 / i2
            | "%" -> i1 % i2
            | "==" -> if i1 = i2 then 1 else 0
            | "!=" -> if i1 <> i2 then 1 else 0
            | "<" -> if i1 < i2 then 1 else 0
            | "<=" -> if i1 <= i2 then 1 else 0
            | ">=" -> if i1 >= i2 then 1 else 0
            | ">" -> if i1 > i2 then 1 else 0
            | _ -> failwith ("unknown primitive " + ope)

        res, store2

    | Call(f, es) -> callfun f es locEnv gloEnv store out

    | PreInc acc ->
        let loc, store1 = access acc locEnv gloEnv store out
        let v = getSto store1 loc + 1
        v, setSto store1 loc v

    | PreDec acc ->
        let loc, store1 = access acc locEnv gloEnv store out
        let v = getSto store1 loc - 1
        v, setSto store1 loc v

    | Ite(c, y, n) ->
        let ce, store = eval c locEnv gloEnv store out

        if ce <> 0 then
            eval y locEnv gloEnv store out
        else
            eval n locEnv gloEnv store out
    | Andalso _ -> failwith "Repalced"
    | Orelse _ -> failwith "Replaced"


and access (acc: access) (locEnv: locEnv) (gloEnv: gloEnv) (store: store) (out: string ref) : int * store =
    match acc with
    | AccVar x -> lookup (fst locEnv) x, store
    | AccDeref e -> eval e locEnv gloEnv store out
    | AccIndex(acc, idx) ->
        let a, store1 = access acc locEnv gloEnv store out
        let aval = getSto store1 a
        let i, store2 = eval idx locEnv gloEnv store1 out
        aval + i, store2

and evals es locEnv gloEnv store out : int list * store =
    match es with
    | [] -> [], store
    | e1 :: er ->
        let v1, store1 = eval e1 locEnv gloEnv store out
        let vr, storer = evals er locEnv gloEnv store1 out
        v1 :: vr, storer

and callfun f es locEnv gloEnv store out : int * store =
    let _, nextloc = locEnv
    let varEnv, funEnv = gloEnv
    let paramdecs, fBody = lookup funEnv f
    let vs, store1 = evals es locEnv gloEnv store out

    let fBodyEnv, store2 = bindVars (List.map snd paramdecs) vs (varEnv, nextloc) store1

    let store3 = exec fBody fBodyEnv gloEnv store2
    -111, store3 out

(* Interpret a complete micro-C program by initializing the store and global environments, then invoking its `main' function. *)

let run (Prog topdecs) vs out =
    let (varEnv, nextloc), funEnv, store0 = initEnvAndStore topdecs
    let mainParams, mainBody = lookup funEnv "main"

    let mainBodyEnv, store1 =
        bindVars (List.map snd mainParams) vs (varEnv, nextloc) store0

    exec mainBody mainBodyEnv (varEnv, funEnv) store1 out
