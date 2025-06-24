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

    let out = proc.StandardOutput.ReadToEnd()

    // Tidy output
    let out = out.Remove(out.Length - 1) // Closing '\n'
    let out = out.Remove(out.LastIndexOf System.Environment.NewLine) // Time info
    let out = out.Trim() // Trailing spaces

    proc.WaitForExit()

    File.Delete pf

    // printfn "Exit: %A" proc.ExitCode

    out

[<Fact>]
let ``machine exists``() =
    Assert.True(File.Exists MACHINE)


[<Fact>]
let ``machine call`` () =
    let e1p = fromFile (Path.Combine(MICROC_DIR, "ex1.c"))

    let e1r = call_machine e1p [ 10 ]
    let e1e = "10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(e1e, e1r)
