(* Polymorphic inference for a higher-order functional language    *)
(* The operator (=) only requires that the arguments have the same type *)
(* sestoft@itu.dk 2010-01-07 *)

module TypeInference

open FunAbsyn

(* Environment operations *)

type 'v env = (string * 'v) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: r -> if x = y then v else lookup r x

(* Operations on sets of type variables, represented as lists.
   Inefficient but simple.  Basically compares type variables
   on their string names.  Correct so long as all type variable names
   are distinct. *)

let rec mem x vs =
    match vs with
    | [] -> false
    | v :: vr -> x = v || mem x vr

(* union(xs, ys) is the set of all elements in xs or ys, without duplicates *)

let rec union (xs, ys) =
    match xs with
    | [] -> ys
    | x :: xr -> if mem x ys then union (xr, ys) else x :: union (xr, ys)

(* unique xs is the set of members of xs, without duplicates *)

let rec unique xs =
    match xs with
    | [] -> []
    | x :: xr -> if mem x xr then unique xr else x :: unique xr

(* A type is int, bool, function, or type variable: *)

type typ =
    | TypI // integers
    | TypB // booleans
    | TypF of typ list * typ // (argumenttype, resulttype)
    | TypV of typevar // type variable

and tyvarkind =
    | NoLink of string // uninstantiated type var.
    | LinkTo of typ // instantiated to typ

and typevar = (tyvarkind * int) ref // kind and binding level

(* A type scheme is a list of generalized type variables, and a type: *)

type typescheme = TypeScheme of typevar list * typ // type variables and type

(* *)

let setTvKind (tyvar: (tyvarkind * int) ref) newKind =
    let _, lvl = tyvar.Value
    tyvar.Value <- newKind, lvl

let setTvLevel (tyvar: (tyvarkind * int) ref) newLevel =
    let kind, _ = tyvar.Value
    tyvar.Value <- kind, newLevel

(* Normalize a type; make type variable point directly to the
   associated type (if any).  This is the `find' operation, with path
   compression, in the union-find algorithm. *)

let rec normType t0 =
    match t0 with
    | TypV tyvar ->
        match tyvar.Value with
        | LinkTo t1, _ ->
            let t2 = normType t1
            setTvKind tyvar (LinkTo t2)
            t2
        | _ -> t0
    | _ -> t0

let rec freeTypeVars t : typevar list =
    match normType t with
    | TypI
    | TypB -> []
    | TypV tv -> [ tv ]
    | TypF(tp, tr) -> // Union parameter and return types
        let freeTypeParams =
            List.fold (fun acc next -> union (acc, freeTypeVars next)) [] tp

        union (freeTypeParams, freeTypeVars tr)

let occurCheck tyvar tyvars =
    if mem tyvar tyvars then
        failwith "type error: circularity"
    else
        ()

let pruneLevel maxLevel tvs =
    let reducelevel (tyvar: (tyvarkind * int) ref) =
        let _, level = tyvar.Value
        setTvLevel tyvar (min level maxLevel)

    List.iter reducelevel tvs

(* Make type variable tyvar equal to type t (by making tyvar link to t),
   but first check that tyvar does not occur in t, and reduce the level
   of all type variables in t to that of tyvar.  This is the `union'
   operation in the union-find algorithm.  *)

let rec linkVarToType (tyvar: typevar) t =
    let _, level = tyvar.Value
    let fvs = freeTypeVars t
    occurCheck tyvar fvs
    pruneLevel level fvs
    setTvKind tyvar (LinkTo t)

let rec typeToString t : string =
    match t with
    | TypI -> "int"
    | TypB -> "bool"
    | TypV _ -> failwith "typeToString impossible"
    | TypF _ -> "function"

(* Unify two types, equating type variables with types as necessary *)

let rec unify t1 t2 : unit =
    let t1' = normType t1
    let t2' = normType t2

    match t1', t2' with
    | TypI, TypI -> ()
    | TypB, TypB -> ()
    | TypF(t1p, t1r), TypF(t2p, t2r) ->
        // Unify paired parameter types.
        let _ = List.map2 (fun t1 t2 -> unify t1 t2) t1p t2p
        unify t1r t2r

    | TypV tv1, TypV tv2 ->
        let _, tv1level = tv1.Value
        let _, tv2level = tv2.Value

        if tv1 = tv2 then ()
        else if tv1level < tv2level then linkVarToType tv1 t2'
        else linkVarToType tv2 t1'
    | TypV tv1, _ -> linkVarToType tv1 t2'
    | _, TypV tv2 -> linkVarToType tv2 t1'
    | TypI, t -> failwith ("type error: int and " + typeToString t)
    | TypB, t -> failwith ("type error: bool and " + typeToString t)
    | TypF _, t -> failwith ("type error: function and " + typeToString t)

(* Generate fresh type variables *)

let tyvarno: int ref = ref 0

let newTypeVar level : typevar =
    let rec mkname i res =
        if i < 26 then
            char (97 + i) :: res
        else
            mkname (i / 26 - 1) (char (97 + i % 26) :: res)

    let intToName i =
        new System.String(Array.ofList ('\'' :: mkname i []))

    tyvarno.Value <- tyvarno.Value + 1
    ref (NoLink(intToName tyvarno.Value), level)

(* Generalize over type variables not free in the context; that is,
   over those whose level is higher than the current level: *)

let rec generalize level (t: typ) : typescheme =
    let notfreeincontext (tyvar: (tyvarkind * int) ref) =
        let _, linkLevel = tyvar.Value
        linkLevel > level

    let tvs = List.filter notfreeincontext (freeTypeVars t)
    TypeScheme(unique tvs, t) // The unique call seems unnecessary because freeTypeVars has no duplicates??

(* Copy a type, replacing bound type variables as dictated by tvenv,
   and non-bound ones by a copy of the type linked to *)

let rec copyType subst t : typ =
    match t with
    | TypV tyvar ->
        let (* Could this be rewritten so that loop does only the substitution *) rec loop subst1 =
            match subst1 with
            | (tyvar1, type1) :: rest -> if tyvar1 = tyvar then type1 else loop rest
            | [] ->
                match tyvar.Value with
                | NoLink _, _ -> t
                | LinkTo t1, _ -> copyType subst t1

        loop subst

    | TypF(tp, tr) ->
        // Copy the type to all parameters
        let tpc = List.map (fun t1 -> copyType subst t1) tp

        TypF(tpc, copyType subst tr)

    | TypI -> TypI
    | TypB -> TypB

(* Create a type from a type scheme (tvs, t) by instantiating all the
   type scheme's parameters tvs with fresh type variables *)

let specialize level (TypeScheme(tvs, t)) : typ =
    let bindfresh tv = tv, TypV(newTypeVar level)

    match tvs with
    | [] -> t
    | _ ->
        let subst = List.map bindfresh tvs
        copyType subst t

(* Pretty-print type, using names 'a, 'b, ... for type variables *)

let rec showType t : string =
    let rec pr t =
        match normType t with
        | TypI -> "int"
        | TypB -> "bool"
        | TypV tyvar ->
            match tyvar.Value with
            | NoLink name, _ -> name
            | _ -> failwith "showType impossible"

        | TypF(tp, tr) ->
            let tps = List.fold (fun acc next -> acc + " " + pr next) "" tp

            "(" + tps + " -> " + pr tr + " )"

    pr t

(* A type environment maps a program variable name to a typescheme *)

type tenv = typescheme env

(* Type inference helper function:
   (typ lvl env e) returns the type of e in env at level lvl *)

let rec typ (lvl: int) (env: tenv) (e: expr) : typ =
    match e with
    | CstI _ -> TypI

    | CstB _ -> TypB

    | Var x -> specialize lvl (lookup env x)

    | Prim(ope, e1, e2) ->
        let t1 = typ lvl env e1
        let t2 = typ lvl env e2

        match ope with
        | "*"
        | "+"
        | "-" ->
            unify TypI t1
            unify TypI t2
            TypI
        | "=" ->
            unify t1 t2
            TypB
        | "<"
        | "&" ->
            unify TypB t1
            unify TypB t2
            TypB

        | _ -> failwith (sprintf "unknown primitive %A" ope)

    | Let(x, eRhs, letBody) ->
        let lvl1 = lvl + 1
        let resTy = typ lvl1 env eRhs
        let letEnv = (x, generalize lvl resTy) :: env
        typ lvl letEnv letBody

    | If(e1, e2, e3) ->
        let t2 = typ lvl env e2
        let t3 = typ lvl env e3
        unify TypB (typ lvl env e1)
        unify t2 t3
        t2

    | Letfun(f, tvs, fBody, letBody) ->

        let lvl1 = lvl + 1
        let fTyp = TypV(newTypeVar lvl1)


        let pTypes = List.map (fun _ -> TypV(newTypeVar lvl1)) tvs

        let pTypeMap =
            List.fold2 (fun acc a b -> (a, TypeScheme([], b)) :: acc) [] tvs pTypes

        let fBodyEnv = pTypeMap @ (f, TypeScheme([], fTyp)) :: env

        let rTyp = typ lvl1 fBodyEnv fBody

        let _ = unify fTyp (TypF(pTypes, rTyp))

        let bodyEnv = (f, generalize lvl fTyp) :: env

        typ lvl bodyEnv letBody

    | Call(eFun, eArgs) ->

        let tf = typ lvl env eFun

        // Map each parameter to its type.
        let tx = List.map (fun ex -> typ lvl env ex) eArgs

        let tr = TypV(newTypeVar lvl)

        unify tf (TypF(tx, tr))

        tr

    | Fun(tvs, fBody) ->
        let lvl1 = lvl + 1

        let pTypes = List.map (fun _ -> TypV(newTypeVar lvl1)) tvs

        let pTypeMap =
            List.fold2 (fun acc a b -> (a, TypeScheme([], b)) :: acc) []  tvs pTypes

        let fBodyEnv = pTypeMap @ env

        let rTyp = typ lvl1 fBodyEnv fBody

        TypF(pTypes, rTyp)



    | Sel(_, _) -> failwith "Not Implemented"

    | Tup _ -> failwith "Not Implemented"

(* Type inference: tyinf e0 returns the type of e0, if any *)

let rec tyinf e0 = typ 0 [] e0

let inferType e =
    tyvarno.Value <- 0
    showType (tyinf e)
