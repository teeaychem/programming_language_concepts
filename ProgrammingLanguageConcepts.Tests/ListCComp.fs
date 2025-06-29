module TestsListC


open Xunit
open ListCParse
open ListCComp
open System.Diagnostics
open System.IO

let LISTC_DIR = Path.Combine(__SOURCE_DIRECTORY__, "ListC")
let MACHINE = Path.Combine(LISTC_DIR, "machine")

let call_machine prg (args: int list) =

    let pf = Path.GetTempFileName()

    let _ = compileToFile prg pf

    let info = new ProcessStartInfo(MACHINE)

    info.Arguments <- sprintf "%s %A" pf (String.concat " " (List.map string args))

    info.RedirectStandardOutput <- true

    use proc = Process.Start info

    let out = ref (proc.StandardOutput.ReadToEnd())

    // Tidy output
    out.Value <- out.Value.Trim() // Trailing spaces

    proc.WaitForExit()

    File.Delete pf

    out.Value

[<Fact>]
let ``machine exists`` () = Assert.True(File.Exists MACHINE)

[<Fact>]
let ``ex30`` () =
    let ep = fromFile (Path.Combine(LISTC_DIR, "ex30.lc"))

    let er = call_machine ep [ 10 ]
    let ee = "10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(ee, er)

[<Fact>]
let ``ex34`` () =
    let ep = fromFile (Path.Combine(LISTC_DIR, "ex34.lc"))

    let er = call_machine ep [ 10 ]
    let ee = "11 33"

    Assert.Equal(ee, er)


[<Fact>]
let ``ex35`` () =
    let ep = fromFile (Path.Combine(LISTC_DIR, "ex35.lc"))

    let er = call_machine ep [ 10 ]
    let ee = "33 33 44 44"

    Assert.Equal(ee, er)


[<Fact>]
let ``ex36`` () =
    let ep = fromFile (Path.Combine(LISTC_DIR, "ex36.lc"))

    let er = call_machine ep [ 10 ]
    let ee = "1 1"

    Assert.Equal(ee, er)
