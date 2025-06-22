(* Lexing and parsing of micro-C programs using fslex and fsyacc *)

module MicroCParse

open System.IO
open FSharp.Text.Lexing
open CAbsyn

(* Plain parsing from a string, with poor error reporting *)

let fromString (str: string) : program =
    let lexbuf = LexBuffer<char>.FromString str

    try
        CPar.Main CLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s near line %d, column %d\n" exn.Message (pos.Line + 1) pos.Column


let fromFile (filename: string) =
    use reader = new StreamReader(filename)
    let lexbuf = LexBuffer<char>.FromTextReader reader

    try
        CPar.Main CLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s in file %s near line %d, column %d\n" exn.Message filename (pos.Line + 1) pos.Column
