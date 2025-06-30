module TestsIcon

open Xunit
open Icon


[<Fact>]
let ``Icon`` () =

    // (write(1 to 3)) ; fail
    let ex1 = Seq(Write(FromTo(1, 3)), Fail)
    let ex1out = ref ""
    let _ = run ex1 ex1out
    Assert.Equal("1 Failed", ex1out.Value) // Sequential composition / Don't backtrack

    // (write(1 to 3)) & fail
    let ex2 = And(Write(FromTo(1, 3)), Fail)
    let ex2out = ref ""
    let _ = run ex2 ex2out
    Assert.Equal("1 2 3 Failed", ex2out.Value) // Cartesian

    // (write((1 to 3) & (4 to 6))) & fail
    let ex3and = And(Write(And(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3andout = ref ""
    let _ = run ex3and ex3andout
    Assert.Equal("4 5 6 4 5 6 4 5 6 Failed", ex3andout.Value)

    // (write((1 to 3) | (4 to 6))) & fail
    let ex3or = And(Write(Or(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3orout = ref ""
    let _ = run ex3or ex3orout
    Assert.Equal("1 2 3 4 5 6 Failed", ex3orout.Value) // Union

    // (write((1 to 3) ; (4 to 6))) & fail
    let ex3seq = And(Write(Seq(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3seqout = ref ""
    let _ = run ex3seq ex3seqout
    Assert.Equal("4 5 6 Failed", ex3seqout.Value)

    // write((1 to 2) & ((4 to 5) & "found"))
    let ex4 = Write(And(FromTo(1, 2), And(FromTo(4, 5), CstS "found")))
    let ex4out = ref ""
    let _ = run ex4 ex4out
    Assert.Equal("found ", ex4out.Value) // No failure to backtrack from


    // write((1 to 3) & ((4 to 6) & "found"))
    let ex4x = Write(And(ex4, Fail))
    let ex4xout = ref ""
    let _ = run ex4x ex4xout
    Assert.Equal("found found found found Failed", ex4xout.Value) // No failure to backtrack from

    // every(write(1 to 3))
    let ex5 = Every(Write(FromTo(1, 3)))
    let ex5out = ref ""
    let _ = run ex5 ex5out
    Assert.Equal("1 2 3 ", ex5out.Value) // Explore every path

    // (every(write(1 to 3)) & (4 to 6))
    let ex6 = And(Every(Write(FromTo(1, 3))), FromTo(4, 6))
    let ex6out = ref ""
    let _ = run ex6 ex6out
    Assert.Equal("1 2 3 ", ex6out.Value)

    // every(write((1 to 3) + (4 to 6)))
    let ex7 = Every(Write(Prim("+", FromTo(1, 3), FromTo(4, 6))))
    let ex7out = ref ""
    let _ = run ex7 ex7out
    Assert.Equal("5 6 7 6 7 8 7 8 9 ", ex7out.Value)

    // write(4 < (1 to 10))
    let ex8 = Write(Prim("<", CstI 4, FromTo(1, 10)))
    let ex8out = ref ""
    let _ = run ex8 ex8out
    Assert.Equal("5 ", ex8out.Value)

    true
