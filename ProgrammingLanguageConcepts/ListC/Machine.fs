(* File ListC/Machine.fs

   Instructions and code emission for a stack-based
   abstract machine with lists and garbage collection
   sestoft@itu.dk 2009-10-18

   An implementation of the machine is found in file ListC/listmachine.c.

   Must precede Comp.fs in the VS Solution Explorer. *)

module ListCMachine

type label = string

type instr =
    | Label of label // symbolic label; pseudo-instruc.
    | CSTI of int // constant
    | ADD // addition
    | SUB // subtraction
    | MUL // multiplication
    | DIV // division
    | MOD // modulus
    | EQ // equality: s[sp-1] == s[sp]
    | LT // less than: s[sp-1] < s[sp]
    | NOT // logical negation:  s[sp] != 0
    | DUP // duplicate stack top
    | SWAP // swap s[sp-1] and s[sp]
    | LDI // get s[s[sp]]
    | STI // set s[s[sp-1]]
    | GETBP // get bp
    | GETSP // get sp
    | INCSP of int // increase stack top by m
    | GOTO of label // go to label
    | IFZERO of label // go to label if s[sp] == 0
    | IFNZRO of label // go to label if s[sp] != 0
    | CALL of int * label // move m args up 1, push pc, jump
    | TCALL of int * int * label // move m args down n, jump
    | RET of int // pop m and return to s[sp]
    | PRINTI // print s[sp] as integer
    | PRINTC // print s[sp] as character
    | LDARGS // load command line args on stack
    | STOP // halt the abstract machine
    | NIL // load nil on stack
    | CONS // create cons cell and load ref.
    | CAR // get first field of cons cell
    | CDR // get second field of cons cell
    | SETCAR // set first field of cons cell
    | SETCDR // set second field of cons cell

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
let CODENIL = 26
let CODECONS = 27
let CODECAR = 28
let CODECDR = 29
let CODESETCAR = 30
let CODESETCDR = 31

(* Bytecode emission, first pass: build environment that maps each label to an integer address in the bytecode. *)

let makelabenv (addr, labenv) instr =
    match instr with
    | Label lab -> addr, (lab, addr) :: labenv
    | CSTI _ -> addr + 2, labenv
    | ADD -> addr + 1, labenv
    | SUB -> addr + 1, labenv
    | MUL -> addr + 1, labenv
    | DIV -> addr + 1, labenv
    | MOD -> addr + 1, labenv
    | EQ -> addr + 1, labenv
    | LT -> addr + 1, labenv
    | NOT -> addr + 1, labenv
    | DUP -> addr + 1, labenv
    | SWAP -> addr + 1, labenv
    | LDI -> addr + 1, labenv
    | STI -> addr + 1, labenv
    | GETBP -> addr + 1, labenv
    | GETSP -> addr + 1, labenv
    | INCSP _ -> addr + 2, labenv
    | GOTO _ -> addr + 2, labenv
    | IFZERO _ -> addr + 2, labenv
    | IFNZRO _ -> addr + 2, labenv
    | CALL _ -> addr + 3, labenv
    | TCALL _ -> addr + 4, labenv
    | RET _ -> addr + 2, labenv
    | PRINTI -> addr + 1, labenv
    | PRINTC -> addr + 1, labenv
    | LDARGS -> addr + 1, labenv
    | STOP -> addr + 1, labenv
    | NIL -> addr + 1, labenv
    | CONS -> addr + 1, labenv
    | CAR -> addr + 1, labenv
    | CDR -> addr + 1, labenv
    | SETCAR -> addr + 1, labenv
    | SETCDR -> addr + 1, labenv

(* Bytecode emission, second pass: output bytecode as integers *)

let rec emitints getlab instr ints =
    match instr with
    | Label _ -> ints
    | CSTI i -> CODECSTI :: i :: ints
    | ADD -> CODEADD :: ints
    | SUB -> CODESUB :: ints
    | MUL -> CODEMUL :: ints
    | DIV -> CODEDIV :: ints
    | MOD -> CODEMOD :: ints
    | EQ -> CODEEQ :: ints
    | LT -> CODELT :: ints
    | NOT -> CODENOT :: ints
    | DUP -> CODEDUP :: ints
    | SWAP -> CODESWAP :: ints
    | LDI -> CODELDI :: ints
    | STI -> CODESTI :: ints
    | GETBP -> CODEGETBP :: ints
    | GETSP -> CODEGETSP :: ints
    | INCSP m -> CODEINCSP :: m :: ints
    | GOTO lab -> CODEGOTO :: getlab lab :: ints
    | IFZERO lab -> CODEIFZERO :: getlab lab :: ints
    | IFNZRO lab -> CODEIFNZRO :: getlab lab :: ints
    | CALL(m, lab) -> CODECALL :: m :: getlab lab :: ints
    | TCALL(m, n, lab) -> CODETCALL :: m :: n :: getlab lab :: ints
    | RET m -> CODERET :: m :: ints
    | PRINTI -> CODEPRINTI :: ints
    | PRINTC -> CODEPRINTC :: ints
    | LDARGS -> CODELDARGS :: ints
    | STOP -> CODESTOP :: ints
    | NIL -> CODENIL :: ints
    | CONS -> CODECONS :: ints
    | CAR -> CODECAR :: ints
    | CDR -> CODECDR :: ints
    | SETCAR -> CODESETCAR :: ints
    | SETCDR -> CODESETCDR :: ints


(* Convert instruction list to int list in two passes:
   Pass 1: build label environment
   Pass 2: output instructions using label environment *)

let code2ints (code: instr list) : int list =
    let _, labenv = List.fold makelabenv (0, []) code
    let getlab lab = lookup labenv lab
    List.foldBack (emitints getlab) code []
