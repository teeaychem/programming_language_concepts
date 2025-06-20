module TestsHigherFun


open Xunit

open FunAbsyn
open HigherFun

[<Fact>]
let ``setup`` () =
    (* Evaluate in empty environment: program must have no free variables: *)

    (* Examples in abstract syntax *)

    let ex1 =
        Letfun("f1", [ "x" ], Prim("+", Var "x", CstI 1), Call(Var "f1", [ CstI 12 ]))

    (* Factorial *)

    let ex2 =
        Letfun(
            "fac",
            [ "x" ],
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call(Var "fac", [ Prim("-", Var "x", CstI 1) ]))),
            Call(Var "fac", [ Var "n" ])
        )

    (* let fac10 = eval ex2 [("n", Int 10)];; *)

    let ex3 =
        Letfun(
            "tw",
            [ "g" ],
            Letfun("app", [ "x" ], Call(Var "g", [ Call(Var "g", [ Var "x" ]) ]), Var "app"),
            Letfun("mul3", [ "y" ], Prim("*", CstI 3, Var "y"), Call(Call(Var "tw", [ Var "mul3" ]), [ CstI 11 ]))
        )

    let ex4 =
        Letfun(
            "tw",
            [ "g" ],
            Letfun("app", [ "x" ], Call(Var "g", [ Call(Var "g", [ Var "x" ]) ]), Var "app"),
            Letfun("mul3", [ "y" ], Prim("*", CstI 3, Var "y"), Call(Var "tw", [ Var "mul3" ]))
        )


    Assert.Equal(Int 13, eval ex1 [])
    Assert.Equal(Int 3628800, eval ex2 [("n", Int 10)])
    Assert.Equal(Int 99, eval ex3 [])
    // Assert.Equal(..., eval ex4 [])
