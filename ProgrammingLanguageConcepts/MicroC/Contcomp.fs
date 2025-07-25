(* File MicroC/Contcomp.fs
   A continuation-based (backwards) compiler from micro-C, a fraction of
   the C language, to an abstract machine.
   sestoft@itu.dk * 2011-11-10

   The abstract machine code is generated backwards, so that jumps to
   jumps can be eliminated, so that tail-calls (calls immediately
   followed by return) can be recognized, dead code can be eliminated,
   etc.

   The compilation of a block, which may contain a mixture of
   declarations and statements, proceeds in two passes:

   Pass 1: elaborate declarations to find the environment in which
           each statement must be compiled; also translate
           declarations into allocation instructions, of type
           bstmtordec.

   Pass 2: compile the statements in the given environments. *)

module Contcomp

open System.IO
open CAbsyn
open Machine

(* The intermediate representation between passes 1 and 2 above:  *)

type bstmtordec =
    | BDec of instr list // Declaration of local variable
    | BStmt of stmt // A statement
    | BDecA of instr list // Declaration and assignment

(* ------------------------------------------------------------------- *)

(* Code-generating functions that perform local optimizations *)

let rec addINCSP m1 C : instr list =
    match C with
    | INCSP m2 :: C1 -> addINCSP (m1 + m2) C1
    | RET m2 :: C1 -> RET(m2 - m1) :: C1
    | Label _lab :: RET m2 :: _ -> RET(m2 - m1) :: C
    | _ -> if m1 = 0 then C else INCSP m1 :: C

let addLabel C : label * instr list = (* Conditional jump to C *)
    match C with
    | Label lab :: _ -> lab, C
    | GOTO lab :: _ -> lab, C
    | _ ->
        let lab = newLabel ()
        lab, Label lab :: C

let makeJump C : instr * instr list = (* Unconditional jump to C *)
    match C with
    | RET m :: _ -> RET m, C
    | Label _lab :: RET m :: _ -> RET m, C
    | Label lab :: _ -> GOTO lab, C
    | GOTO lab :: _ -> GOTO lab, C
    | _ ->
        let lab = newLabel ()
        GOTO lab, Label lab :: C

let makeCall m lab C : instr list =
    match C with
    | RET n :: C1 -> TCALL(m, n, lab) :: C1
    | Label _ :: RET n :: _ -> TCALL(m, n, lab) :: C
    | _ -> CALL(m, lab) :: C

let rec deadcode C =
    match C with
    | [] -> []
    | Label _lab :: _ -> C
    | _ :: C1 -> deadcode C1

let addIFZERO labse C : instr list =
    match C with
    | GOTO _ :: Label labc :: C1 ->
        if labse = labc then
            IFNZRO labc :: Label labse :: C1
        else
            IFZERO labse :: C
    | _ -> IFZERO labse :: C


let addIFNZRO labse C : instr list =
    match C with
    | GOTO _ :: Label labc :: C1 ->
        if labse = labc then
            IFZERO labc :: Label labse :: C1
        else
            IFNZRO labse :: C
    | _ -> IFNZRO labse :: C

let addNOT C =
    match C with
    | NOT :: C1 -> C1
    | IFZERO lab :: C1 -> addIFNZRO lab C1
    | IFNZRO lab :: C1 -> addIFZERO lab C1
    | _ -> NOT :: C


let addJump jump C = (* jump is GOTO or RET *)
    let C1 = deadcode C

    match jump, C1 with
    | GOTO lab1, Label lab2 :: _ -> if lab1 = lab2 then C1 else GOTO lab1 :: C1
    | _ -> jump :: C1

let addGOTO lab C = addJump (GOTO lab) C

let rec addCST i C =
    match i, C with
    | 0, ADD :: C1 -> C1
    | 0, SUB :: C1 -> C1
    | 0, NOT :: C1 -> addCST 1 C1
    | _, NOT :: C1 -> addCST 0 C1
    | 1, MUL :: C1 -> C1
    | 1, DIV :: C1 -> C1
    | 0, EQ :: C1 -> addNOT C1
    | _, INCSP m :: C1 -> if m < 0 then addINCSP (m + 1) C1 else CSTI i :: C
    | 0, IFZERO lab :: C1 -> addGOTO lab C1
    | _, IFZERO _lab :: C1 -> C1
    | 0, IFNZRO _lab :: C1 -> C1
    | _, IFNZRO lab :: C1 -> addGOTO lab C1

    | i, CSTI j :: EQ :: NOT :: C1 -> if i = j then addCST 0 C1 else addCST 1 C1
    | i, CSTI j :: EQ :: C1 -> if i = j then addCST 1 C1 else addCST 0 C1
    | i, CSTI j :: LT :: NOT :: C1 -> if i < j then addCST 0 C1 else addCST 1 C1
    | i, CSTI j :: SWAP :: LT :: NOT :: C1 -> if j < i then addCST 0 C1 else addCST 1 C1
    | i, CSTI j :: LT :: C1 -> if i < j then addCST 1 C1 else addCST 0 C1
    | i, CSTI j :: SWAP :: LT :: C1 -> if j < i then addCST 1 C1 else addCST 0 C1

    | _ -> CSTI i :: C

(* ------------------------------------------------------------------- *)

(* Simple environment operations *)

type 'data env = (string * 'data) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: yr -> if x = y then v else lookup yr x

(* A global variable has an absolute address, a local one has an offset: *)

type var =
    | Glovar of int // absolute address in stack
    | Locvar of int // address relative to bottom of frame

(* The variable environment keeps track of global and local variables, and keeps track of next available offset for local variables *)

type varEnv = (var * typ) env * int

(* The function environment maps a function name to the function's label, its return type, and its parameter declarations *)

type paramdecs = (typ * string) list
type funEnv = (label * typ option * paramdecs) env

(* Bind declared variable in varEnv and generate code to allocate it: *)

let allocate (kind: int -> var) (typ, x) (varEnv: varEnv) : varEnv * instr list =
    let env, fdepth = varEnv

    match typ with
    | TypA(TypA _, _) -> failwith "allocate: arrays of arrays not permitted"
    | TypA(_t, Some i) ->
        let newEnv = (x, (kind (fdepth + i), typ)) :: env, fdepth + i + 1
        let code = [ INCSP i; GETSP; CSTI(i - 1); SUB ]
        newEnv, code
    | _ ->
        let newEnv = (x, (kind fdepth, typ)) :: env, fdepth + 1
        let code = [ INCSP 1 ]
        newEnv, code

(* Bind declared parameter in env: *)

let bindParam (env, fdepth) (typ, x) : varEnv =
    (x, (Locvar fdepth, typ)) :: env, fdepth + 1

let bindParams paras (env, fdepth) : varEnv = List.fold bindParam (env, fdepth) paras

(* ------------------------------------------------------------------- *)

(* Build environments for global variables and global functions *)

let makeGlobalEnvs (topdecs: topdec list) : varEnv * funEnv * instr list =
    let rec addv decs varEnv funEnv =
        match decs with
        | [] -> varEnv, funEnv, []
        | dec :: decr ->
            match dec with
            | Vardec(typ, x) ->
                let varEnv1, code1 = allocate Glovar (typ, x) varEnv
                let varEnvr, funEnvr, coder = addv decr varEnv1 funEnv
                varEnvr, funEnvr, code1 @ coder

            | Fundec(tyOpt, f, xs, _body) -> addv decr varEnv ((f, (newLabel (), tyOpt, xs)) :: funEnv)

            | VardecA(_, _, _) -> failwith "Not Implemented"

    addv topdecs ([], 0) []

(* ------------------------------------------------------------------- *)

(* Compiling micro-C statements:

   * stmt    is the statement to compile
   * varenv  is the local and global variable environment
   * funEnv  is the global function environment
   * C       is the code that follows the code for stmt *)

let rec cStmt stmt (varEnv: varEnv) (funEnv: funEnv) (C: instr list) : instr list =
    match stmt with
    | If(e, stmt1, stmt2) ->
        let jumpend, C1 = makeJump C
        let labelse, C2 = addLabel (cStmt stmt2 varEnv funEnv C1)
        cExpr e varEnv funEnv (addIFZERO labelse (cStmt stmt1 varEnv funEnv (addJump jumpend C2)))

    | While(e, body) ->
        let labbegin = newLabel ()
        let jumptest, C1 = makeJump (cExpr e varEnv funEnv (addIFNZRO labbegin C))
        addJump jumptest (Label labbegin :: cStmt body varEnv funEnv C1)

    | Expr e -> cExpr e varEnv funEnv (addINCSP -1 C)

    | Block stmts ->
        let rec pass1 stmts (_, fdepth as varEnv) =
            match stmts with
            | [] -> [], fdepth
            | s1 :: sr ->
                let _, varEnv1 as res1 = bStmtordec s1 varEnv funEnv
                let resr, fdepthr = pass1 sr varEnv1
                res1 :: resr, fdepthr

        let stmtsback, fdepthend = pass1 stmts varEnv

        let rec pass2 pairs C =
            match pairs with
            | [] -> C
            | (BDec code, _varEnv) :: sr -> code @ pass2 sr C
            | (BDecA code, _varEnv) :: sr -> code @ pass2 sr C
            | (BStmt stmt, varEnv) :: sr -> cStmt stmt varEnv funEnv (pass2 sr C)


        pass2 stmtsback (addINCSP (snd varEnv - fdepthend) C)

    | Return None -> RET(snd varEnv - 1) :: deadcode C

    | Return(Some e) -> cExpr e varEnv funEnv (RET(snd varEnv) :: deadcode C)

and bStmtordec stmtOrDec (varEnv: varEnv) (funEnv: funEnv) : bstmtordec * varEnv =
    match stmtOrDec with

    | Stmt stmt -> BStmt stmt, varEnv

    | Dec(typ, x) ->
        let varEnv1, code = allocate Locvar (typ, x) varEnv
        BDec code, varEnv1

    | DecA(typ, acc, expr) ->
        let varEnv, Calloc = allocate Locvar (typ, acc) varEnv

        let C = [ STI; INCSP -1 ] // Discard the value stored
        let C1 = cExpr expr varEnv funEnv C
        let C2 = cAccess (AccVar acc) varEnv funEnv C1

        BDecA(Calloc @ C2), varEnv

(* Compiling micro-C expressions:

   * e       is the expression to compile
   * varEnv  is the compile-time variable environment
   * funEnv  is the compile-time environment
   * C       is the code following the code for this expression

   Net effect principle:
   If: The compilation (cExpr e varEnv funEnv C) of expression e returns the instruction sequence instrs,
   Then: the execution of instrs will have the same effect as an instruction sequence that first computes the value of expression e on the stack top and then executes C,
   But: Because of optimizations instrs may actually achieve this in a different way.  *)

and cExpr (e: expr) (varEnv: varEnv) (funEnv: funEnv) (C: instr list) : instr list =
    match e with
    | Access acc -> cAccess acc varEnv funEnv (LDI :: C)

    | Assign(acc, e) ->

        match e with
        | Prim2(op, Access v, e1)
        | Prim2(op, e1, Access v) when acc = v ->

            let opc =
                match op with
                | "*" -> MUL
                | "+" -> ADD
                | "-" -> SUB
                | "/" -> DIV
                | "%" -> MOD
                | _ -> failwith "Unsupported"

            let C = opc :: STI :: C // apply the op and store
            let C = cExpr e1 varEnv funEnv C // get the value of e1 on the stack
            let C = DUP :: LDI :: C // duplicate location for write and read, read
            let C = cAccess acc varEnv funEnv C // acc location on the stack

            C

        | _ -> cAccess acc varEnv funEnv (cExpr e varEnv funEnv (STI :: C))

    | CstI i -> addCST i C

    | Addr acc -> cAccess acc varEnv funEnv C

    | Prim1(ope, e1) ->
        cExpr
            e1
            varEnv
            funEnv
            (match ope with
             | "!" -> addNOT C
             | "printi" -> PRINTI :: C
             | "printc" -> PRINTC :: C
             | _ -> failwith "unknown primitive 1")

    | Prim2(ope, e1, e2) ->
        cExpr
            e1
            varEnv
            funEnv
            (cExpr
                e2
                varEnv
                funEnv
                (match ope with
                 | "*" -> MUL :: C
                 | "+" -> ADD :: C
                 | "-" -> SUB :: C
                 | "/" -> DIV :: C
                 | "%" -> MOD :: C
                 | "==" -> EQ :: C
                 | "!=" -> EQ :: addNOT C
                 | "<" -> LT :: C
                 | ">=" -> LT :: addNOT C
                 | ">" -> SWAP :: LT :: C
                 | "<=" -> SWAP :: LT :: addNOT C
                 | _ -> failwith "unknown primitive 2"))

    | Call(f, es) -> callfun f es varEnv funEnv C

    | Ite(e1, e2, e3) ->
        let labelc, C = addLabel C // L2:
        let labele, C = addLabel (cExpr e3 varEnv funEnv C) // L1: <e3>
        let C = addGOTO labelc C // GOTO L2
        let C = cExpr e2 varEnv funEnv C // <e2>
        let C = addIFZERO labele C // IFZERO L1
        cExpr e1 varEnv funEnv C // <e1>

    | PreDec acc -> cAccess acc varEnv funEnv (DUP :: LDI :: CSTI 1 :: SUB :: STI :: C)
    | PreInc acc -> cAccess acc varEnv funEnv (DUP :: LDI :: CSTI 1 :: ADD :: STI :: C)

    | Andalso _ -> failwith "Replaced"
    | Orelse _ -> failwith "Replaced"


(* Generate code to access variable, dereference pointer or index array: *)

and cAccess (access: access) (varEnv: varEnv) (funEnv: funEnv) C : instr list =
    match access with
    | AccVar x ->
        match lookup (fst varEnv) x with
        | Glovar addr, _ -> addCST addr C
        | Locvar addr, _ -> GETBP :: addCST addr (ADD :: C)
    | AccDeref e -> cExpr e varEnv funEnv C
    | AccIndex(acc, idx) -> cAccess acc varEnv funEnv (LDI :: cExpr idx varEnv funEnv (ADD :: C))

(* Generate code to evaluate a list es of expressions: *)

and cExprs es varEnv funEnv C =
    match es with
    | [] -> C
    | e1 :: er -> cExpr e1 varEnv funEnv (cExprs er varEnv funEnv C)

(* Generate code to evaluate arguments es and then call function f: *)

and callfun (f: string) (es: expr list) (varEnv: varEnv) (funEnv: funEnv) C : instr list =
    let labf, _tyOpt, paramdecs = lookup funEnv f
    let argc = List.length es

    if argc = List.length paramdecs then
        cExprs es varEnv funEnv (makeCall argc labf C)
    else
        failwith (f + ": parameter/argument mismatch")

(* Compile a complete micro-C program: globals, call to main, functions *)

let cProgram (Prog topdecs) : instr list =
    let _ = resetLabels ()
    let (globalVarEnv, _), funEnv, globalInit = makeGlobalEnvs topdecs

    let compilefun (_tyOpt, f, _xs, body) =
        let labf, _, paras = lookup funEnv f
        let envf, fdepthf = bindParams paras (globalVarEnv, 0)
        let C0 = [ RET(List.length paras - 1) ]
        let code = cStmt body (envf, fdepthf) funEnv C0
        Label labf :: code

    let functions =
        List.choose
            (function
            | Fundec(rTy, name, argTy, body) -> Some(compilefun (rTy, name, argTy, body))
            | Vardec _ -> None
            | VardecA(_, _, _) -> None)
            topdecs

    let mainlab, _, mainparams = lookup funEnv "main"
    let argc = List.length mainparams
    globalInit @ [ LDARGS; CALL(argc, mainlab); STOP ] @ List.concat functions

(* Compile the program (in abstract syntax) and write it to file fname; also, return the program as a list of instructions.  *)

let contCompile program = code2ints (cProgram program)

let intsToFile (inss: int list) (fname: string) =
    File.WriteAllText(fname, String.concat " " (List.map string inss))

let contCompileToFile program fname =
    let instrs = cProgram program
    let bytecode = code2ints instrs
    intsToFile bytecode fname
    instrs
