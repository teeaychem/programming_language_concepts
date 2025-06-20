module TestsTypedFun

open Xunit
open TypedFun

[<Fact>]
let ``initial`` () =
    (* Examples of successful type checking *)

    let ex1 =
        Letfun("f1", [ "x", TypI ], Prim("+", Var "x", CstI 1), TypI, Call(Var "f1", [ CstI 12 ]))

    Assert.Equal(Int 13, eval ex1 [])

    (* Factorial *)

    let ex2 =
        Letfun(
            "fac",
            [ "x", TypI ],
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call(Var "fac", [ Prim("-", Var "x", CstI 1) ]))),
            TypI,
            Let("n", CstI 7, Call(Var "fac", [ Var "n" ]))
        )

    Assert.Equal(Int 5040, eval ex2 [])



    let ex3 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", CstI 3, CstI 4))

    Assert.Equal(Int 4, eval ex3 [])

    let ex4 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", Var "b", CstB false))

    Assert.Equal(Bool false, eval ex4 [])

    let ex5 = If(Prim("=", CstI 11, CstI 12), CstI 111, CstI 666)

    Assert.Equal(Int 666, eval ex5 [])

    let ex6 =
        Letfun("inf", [ "x", TypI ], Call(Var "inf", [ Var "x" ]), TypI, Call(Var "inf", [ CstI 0 ]))

    let exs = [ ex1; ex2; ex3; ex4; ex5; ex6 ]
    let ext = [ TypI; TypI; TypI; TypB; TypI; TypI ]

    List.map (fun (expected, actual) -> Assert.Equal(expected, typeCheck actual)) (List.zip ext exs)
    |> ignore


    (* Examples of type errors; should throw exception when run: *)

    let exErr1 = Let("b", Prim("=", CstI 1, CstI 2), If(Var "b", Var "b", CstI 6))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr1 |> ignore) |> ignore

    let exErr2 =
        Letfun("f", [ "x", TypB ], If(Var "x", CstI 11, CstI 22), TypI, Call(Var "f", [ CstI 0 ]))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr2 |> ignore) |> ignore

    let exErr3 =
        Letfun("f", [ "x", TypB ], Call(Var "f", [ CstI 22 ]), TypI, Call(Var "f", [ CstB true ]))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr3 |> ignore) |> ignore

    let exErr4 =
        Letfun("f", [ "x", TypB ], If(Var "x", CstI 11, CstI 22), TypB, Call(Var "f", [ CstB true ]))

    Assert.Throws<System.Exception>(fun () -> typeCheck exErr4 |> ignore) |> ignore

[<Fact>]
let ``Exercise 4.7`` () =

    let pow =
        Letfun(
            "pow",
            [ "x", TypI; "n", TypI ],
            If(
                Prim("=", Var "n", CstI 0),
                CstI 1,
                Prim("*", Var "x", Call(Var "pow", [ Var "x"; Prim("-", Var "n", CstI 1) ]))
            ),
            TypI,
            Call(Var "pow", [ CstI 3; CstI 8 ])
        )


    Assert.Equal(TypI, typeCheck pow)
    Assert.Equal(Int 6561, eval pow [])

[<Fact>]
let ``Exercise 4.8`` () =
    let list1 = Conc(CstI 2, Conc(CstI 1, CstN))
    let list2 = Conc(CstI 4, Conc(CstI 3, Conc(CstI 2, CstN)))

    let head_one l =
        Match(
            l,
            CstB false,
            ("h", "t", Letfun("f", [], If(Prim("=", Var "h", CstI 1), CstB true, CstB false), TypB, Call(Var "f", [])))
        )


    Assert.Equal(Bool false, eval (head_one list1) [])

    let find_one l =
        Letfun(
            "f",
            [ "x", TypL TypI ],
            Match(
                Var "x",
                CstB false,
                ("h", "t", If(Prim("=", Var "h", CstI 1), CstB true, Call(Var "f", [ Var "t" ])))
            ),
            TypB,
            Call(Var "f", [ l ])
        )




    Assert.Equal(Bool true, eval (find_one list1) [])
    Assert.Equal(Bool false, eval (find_one list2) [])

    true


[<Fact>]
let ``Exercise 4.8 / 4.9 / 5.7`` () =

    let listI = Conc(CstI 1, Conc(CstI 2, CstN))
    let listIV = Conc(CstI 1, Conc(Var "x", CstN))
    let listIB = Conc(CstI 1, Conc(CstB false, CstN))

    Assert.Equal(TypL TypI, typ listI [])

    Assert.Equal(TypL TypI, typ listIV [ "x", TypI ])

    Assert.Throws<System.Exception>(fun () -> typ listIB [] |> ignore) |> ignore
