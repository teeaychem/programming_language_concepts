module TestsMicroC

open Xunit

open MicroCParse

let run = Interp.run


[<Fact>]
let ``Exercise 7.1`` () =
    let e1out = ref ""
    let e1p = fromFile "MicroC/ex1.c"
    let _e1 = run e1p [ 17 ] e1out
    e1out.Value <- e1out.Value.Trim()

    let e1expected = "17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(e1expected, e1out.Value)

    let e2out = ref ""
    let e2p = fromFile "MicroC/ex2.c"
    let _e2 = run e2p [] e2out
    e2out.Value <- e2out.Value.Trim()

    let e2expected = "-1 -1 1 -999 1 227 12 12 14 114 2 1 1"

    Assert.Equal(e2expected, e2out.Value)
