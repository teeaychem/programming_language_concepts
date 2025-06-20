module TestsHigherFun


open Xunit

open FunParse
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

    let _ex4 =
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


[<Fact>]
let ``Exercise 6.1`` () =
    let e1 = fromString "
let add x = let f y = x+y in f end in (add 2) 5 end
"
    printfn "%A" e1

    let e2 = fromString "
let add x = let f y = x+y in f end
in let addtwo = add 2
    in addtwo 5 end
end"

    let e3 = fromString "let add x = let f y = x+y in f end
in let addtwo = add 2
in let x = 77 in addtwo 5 end
end
end"

// let add x = let f y = x + y in f
// let addTwo = add 2
// add 2 >> let f y = 2 + y in f >> f y
// addtwo 5 >> f 5 >> 7

// addtwo 5
// (add 2) 5
// f 5
// 2 + 5

// etc.

    let _e4 = fromString "let add x = let f y = x+y in f end
in add 2 end"

    Assert.Equal(Int 7, eval e1 [])
    Assert.Equal(Int 7, eval e2 [])
    Assert.Equal(Int 7, eval e3 [])
    // Assert.Equal(..., eval e4 [])

    true
