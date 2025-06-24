module TestsMicroCComp

open Xunit
open MicroCParse
open MicroCComp
open System.Diagnostics
open System.IO


let MICROC_DIR = Path.Combine(__SOURCE_DIRECTORY__, "MicroC")
let MACHINE = Path.Combine(MICROC_DIR, "machine")

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
let ``machine call`` () =
    let e1p = fromFile (Path.Combine(MICROC_DIR, "ex1.c"))

    let e1r = call_machine e1p [ 10 ]
    let e1e = "10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(e1e, e1r)
