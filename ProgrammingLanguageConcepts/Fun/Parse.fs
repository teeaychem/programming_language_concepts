(* Lexing and parsing of micro-ML programs using fslex and fsyacc *)

module FunParse

open System.IO
open FSharp.Text.Lexing
open FunAbsyn

(* Plain parsing from a string, with poor error reporting *)

let fromString (str: string) : expr =
    let lexbuf = LexBuffer<char>.FromString str

    try
        FunPar.Main FunLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s near line %d, column %d\n" exn.Message (pos.Line + 1) pos.Column

(* Parsing from a file *)

let fromFile (filename: string) =
    use reader = new StreamReader(filename)
    let lexbuf = LexBuffer<char>.FromTextReader reader

    try
        FunPar.Main FunLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s in file %s near line %d, column %d\n" exn.Message filename (pos.Line + 1) pos.Column
