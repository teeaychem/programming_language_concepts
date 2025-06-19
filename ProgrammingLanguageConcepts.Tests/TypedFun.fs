module TestsTypedFun

open Xunit
open TypedFun

[<Fact>]
let ``initial`` () =
    (* Examples of successful type checking *)

    let ex1 =
        Letfun("f1", "x", TypI, Prim("+", Var "x", CstI 1), TypI, Call(Var "f1", CstI 12))

    Assert.Equal(13, eval ex1 [])

    (* Factorial *)

    let ex2 =
        Letfun(
            "fac",
            "x",
            TypI,
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call(Var "fac", Prim("-", Var "x", CstI 1)))),
            TypI,
            Let("n", CstI 7, Call(Var "fac", Var "n"))
        )

    Assert.Equal(5040, eval ex2 [])



    let ex3 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", CstI 3, CstI 4))

    Assert.Equal(4, eval ex3 [])

    let ex4 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", Var "b", CstB false))

    Assert.Equal(0, eval ex4 [])

    let ex5 = If(Prim("=", CstI 11, CstI 12), CstI 111, CstI 666)

    Assert.Equal(666, eval ex5 [])

    let ex6 =
        Letfun("inf", "x", TypI, Call(Var "inf", Var "x"), TypI, Call(Var "inf", CstI 0))

    let exs = [ ex1; ex2; ex3; ex4; ex5; ex6 ]
    let ext = [ TypI; TypI; TypI; TypB; TypI; TypI ]

    List.map (fun (expected, actual) -> Assert.Equal(expected, typeCheck actual)) (List.zip ext exs)
    |> ignore


    (* Examples of type errors; should throw exception when run: *)

    let exErr1 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", Var "b", CstI 6))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr1 |> ignore) |> ignore

    let exErr2 =
        Letfun("f", "x", TypB, If(Var "x", CstI 11, CstI 22), TypI, Call(Var "f", CstI 0))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr2 |> ignore) |> ignore

    let exErr3 =
        Letfun("f", "x", TypB, Call(Var "f", CstI 22), TypI, Call(Var "f", CstB true))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr3 |> ignore) |> ignore

    let exErr4 =
        Letfun("f", "x", TypB, If(Var "x", CstI 11, CstI 22), TypB, Call(Var "f", CstB true))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr4 |> ignore) |> ignore
