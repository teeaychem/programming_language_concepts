(* File MicroC/Machine.fs

   Instructions and code emission for a stack-based
   abstract machine * sestoft@itu.dk 2009-09-23

   Implementations of the machine are found in file MicroC/Machine.java
   and MicroC/machine.c.

   Must precede Comp.fs and Contcomp.fs in the VS Solution Explorer. *)

module Machine

type label = string

type instr =
    | ADD // addition
    | CALL of int * label // move m args up 1, push pc, jump
    | CSTI of int // constant
    | DIV // division
    | DUP // duplicate stack top
    | EQ // equality: s[sp-1] == s[sp]
    | GETBP // get bp
    | GETSP // get sp
    | GOTO of label // go to label
    | IFNZRO of label // go to label if s[sp] != 0
    | IFZERO of label // go to label if s[sp] == 0
    | INCSP of int // increase stack top by m
    | LDARGS // load command line args on stack
    | LDI // get s[s[sp]]
    | LT // less than: s[sp-1] < s[sp]
    | Label of label // symbolic label; pseudo-instruc.
    | MOD // modulus
    | MUL // multiplication
    | NOT // logical negation:  s[sp] != 0
    | PRINTC // print s[sp] as character
    | PRINTI // print s[sp] as integer
    | RET of int // pop m and return to s[sp]
    | STI // set s[s[sp-1]]
    | STOP // halt the abstract machine
    | SUB // subtraction
    | SWAP // swap s[sp-1] and s[sp]
    | TCALL of int * int * label // move m args down n, jump

(* Generate new distinct labels *)

let resetLabels, newLabel =
    let lastlab = ref -1

    (fun () -> lastlab.Value <- 0),
    (fun () ->
        lastlab.Value <- 1 + lastlab.Value
        "L" + lastlab.Value.ToString())

(* Simple environment operations *)

type 'data env = (string * 'data) list

let rec lookup env x =
    match env with
    | [] -> failwith (x + " not found")
    | (y, v) :: yr -> if x = y then v else lookup yr x

(* An instruction list is emitted in two phases:
   * pass 1 builds an environment labenv mapping labels to addresses
   * pass 2 emits the code to file, using the environment labenv to resolve labels *)

(* These numeric instruction codes must agree with Machine.java: *)

let CODECSTI = 0
let CODEADD = 1
let CODESUB = 2
let CODEMUL = 3
let CODEDIV = 4
let CODEMOD = 5
let CODEEQ = 6
let CODELT = 7
let CODENOT = 8
let CODEDUP = 9
let CODESWAP = 10
let CODELDI = 11
let CODESTI = 12
let CODEGETBP = 13
let CODEGETSP = 14
let CODEINCSP = 15
let CODEGOTO = 16
let CODEIFZERO = 17
let CODEIFNZRO = 18
let CODECALL = 19
let CODETCALL = 20
let CODERET = 21
let CODEPRINTI = 22
let CODEPRINTC = 23
let CODELDARGS = 24
let CODESTOP = 25

(* Bytecode emission, first pass: build environment that maps each label to an integer address in the bytecode. *)

let makelabenv (addr, labenv) (instr: instr) : int * (label * int) list =
    match instr with
    | ADD -> addr + 1, labenv
    | CALL(_m, _lab) -> addr + 3, labenv
    | CSTI _i -> addr + 2, labenv
    | DIV -> addr + 1, labenv
    | DUP -> addr + 1, labenv
    | EQ -> addr + 1, labenv
    | GETBP -> addr + 1, labenv
    | GETSP -> addr + 1, labenv
    | GOTO _lab -> addr + 2, labenv
    | IFNZRO _lab -> addr + 2, labenv
    | IFZERO _lab -> addr + 2, labenv
    | INCSP _m -> addr + 2, labenv
    | LDARGS -> addr + 1, labenv
    | LDI -> addr + 1, labenv
    | LT -> addr + 1, labenv
    | Label lab -> addr, (lab, addr) :: labenv
    | MOD -> addr + 1, labenv
    | MUL -> addr + 1, labenv
    | NOT -> addr + 1, labenv
    | PRINTC -> addr + 1, labenv
    | PRINTI -> addr + 1, labenv
    | RET _m -> addr + 2, labenv
    | STI -> addr + 1, labenv
    | STOP -> addr + 1, labenv
    | SUB -> addr + 1, labenv
    | SWAP -> addr + 1, labenv
    | TCALL(_m, _n, _lab) -> addr + 4, labenv

(* Bytecode emission, second pass: output bytecode as integers *)

let rec emitints getlab instr ints =
    match instr with
    | ADD -> CODEADD :: ints
    | CALL(m, lab) -> CODECALL :: m :: getlab lab :: ints
    | CSTI i -> CODECSTI :: i :: ints
    | DIV -> CODEDIV :: ints
    | DUP -> CODEDUP :: ints
    | EQ -> CODEEQ :: ints
    | GETBP -> CODEGETBP :: ints
    | GETSP -> CODEGETSP :: ints
    | GOTO lab -> CODEGOTO :: getlab lab :: ints
    | IFNZRO lab -> CODEIFNZRO :: getlab lab :: ints
    | IFZERO lab -> CODEIFZERO :: getlab lab :: ints
    | INCSP m -> CODEINCSP :: m :: ints
    | LDARGS -> CODELDARGS :: ints
    | LDI -> CODELDI :: ints
    | LT -> CODELT :: ints
    | Label _lab -> ints
    | MOD -> CODEMOD :: ints
    | MUL -> CODEMUL :: ints
    | NOT -> CODENOT :: ints
    | PRINTC -> CODEPRINTC :: ints
    | PRINTI -> CODEPRINTI :: ints
    | RET m -> CODERET :: m :: ints
    | STI -> CODESTI :: ints
    | STOP -> CODESTOP :: ints
    | SUB -> CODESUB :: ints
    | SWAP -> CODESWAP :: ints
    | TCALL(m, n, lab) -> CODETCALL :: m :: n :: getlab lab :: ints

(* Convert instruction list to int list in two passes:
   Pass 1: build label environment
   Pass 2: output instructions using label environment *)

let code2ints (code: instr list) : int list =
    let _, labenv = List.fold makelabenv (0, []) code
    let getlab lab = lookup labenv lab
    List.foldBack (emitints getlab) code []
