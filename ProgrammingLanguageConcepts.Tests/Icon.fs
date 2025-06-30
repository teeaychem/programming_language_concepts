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


[<Fact>]
let ``Exercise 11.8(i)`` () =
    let ex = Every(Write(Prim("+", CstI 1, Prim("*", CstI 2, FromTo(1, 4)))))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("3 5 7 9 ", exout.Value)


    let ex = And(And(Write(CstI 21), Write(CstI 22)), Write(CstS "…"))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("21 22 … ", exout.Value)

    let ex =
        Every(Write(Prim("+", Prim("*", CstI 10, FromTo(2, 4)), Prim("+", CstI 1, FromTo(0, 1)))))

    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("21 22 31 32 41 42 ", exout.Value)



[<Fact>]
let ``Exercise 11.8(ii)`` () =
    let ex = Write(Prim("<", CstI 50, Prim("*", CstI 7, FromTo(1,100))))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("56 ", exout.Value)


[<Fact>]
let ``Exercise 11.8(iii)`` () =
    let ex = Every(Write(Prim1("sqr", FromTo(3,6))))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("9 16 25 36 ", exout.Value)

    let ex = Every(Write(Prim1("even", FromTo(1,7))))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("2 4 6 ", exout.Value)


[<Fact>]
let ``Exercise 11.8(iv)`` () =
    // Bounded to ten instances for testing
    let ex = Every(Write(Prim1("multiples", FromTo(3,5))))
    let exout = ref ""
    let _ = run ex exout
    Assert.Equal("0 3 6 9 12 15 18 21 24 27 30 ", exout.Value)
