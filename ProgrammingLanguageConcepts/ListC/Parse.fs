(* Lexing and parsing of list-C programs using fslex and fsyacc *)

module ListCParse

open System.IO
open FSharp.Text.Lexing
open ListCAbsyn

(* Plain parsing from a string, with poor error reporting *)

let fromString (str: string) : program =
    let lexbuf = LexBuffer<char>.FromString str

    try
        ListCPar.Main ListCLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s near line %d, column %d\n" exn.Message (pos.Line + 1) pos.Column

(* Parsing from a file *)

let fromFile (filename: string) =
    use reader = new StreamReader(filename)
    let lexbuf = LexBuffer<char>.FromTextReader reader

    try
        ListCPar.Main ListCLex.Token lexbuf
    with exn ->
        let pos = lexbuf.EndPos
        failwithf "%s in file %s near line %d, column %d\n" exn.Message filename (pos.Line + 1) pos.Column
