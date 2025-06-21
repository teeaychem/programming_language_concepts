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
    Assert.Equal(Int 3628800, eval ex2 [ ("n", Int 10) ])
    Assert.Equal(Int 99, eval ex3 [])
// Assert.Equal(..., eval ex4 [])


[<Fact>]
let ``parse and run`` () =
    let ex5 =
        fromString
            @"let tw g = let app x = g (g x) in app end
       in let mul3 x = 3 * x
       in let quad = tw mul3
       in quad 7 end end end"

    let ex6 =
        fromString
            @"let tw g = let app x = g (g x) in app end
       in let mul3 x = 3 * x
       in let quad = tw mul3
       in quad end end end"

    let ex7 =
        fromString
            @"let rep n =
           let rep1 g =
               let rep2 x = if n=0 then x else ((rep (n-1)) g) (g x)
               in rep2 end
           in rep1 end
       in let mul3 x = 3 * x
       in let tw = rep 2
       in let quad = tw mul3
       in quad 7 end end end end"

    let ex8 =
        fromString
            @"let rep n =
           let rep1 g =
               let rep2 x = if n=0 then x else ((rep (n-1)) g) (g x)
               in rep2 end
           in rep1 end
       in let mul3 x = 3 * x
       in let twototen = (rep 10) mul3
       in twototen 7 end end end"

    let ex8sharp =
        let rec rep n =
            let rep1 g =
                let rep2 x =
                    if n = 0 then x else rep (n - 1) g (g x) in

                rep2 in

            rep1 in

        let mul3 x = 3 * x in
        let twototen = rep 10 mul3 in
        twototen 7


    Assert.Equal(Int 63, eval ex5 [])
    Assert.Equal(Int 63, eval (Call(ex6, [ CstI 7 ])) [])
    Assert.Equal(Int 63, eval ex7 [])
    Assert.Equal(Int ex8sharp, eval ex8 [])


[<Fact>]
let ``Exercise 6.1`` () =
    let e1 =
        fromString
            "
let add x = let f y = x+y in f end in (add 2) 5 end
"

    let e2 =
        fromString
            "
let add x = let f y = x+y in f end
in let addtwo = add 2
    in addtwo 5 end
end"

    let e3 =
        fromString
            "let add x = let f y = x+y in f end
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

    let _e4 =
        fromString
            "
let add x = let f y = x+y in f end
in add 2 end"

    Assert.Equal(Int 7, eval e1 [])
    Assert.Equal(Int 7, eval e2 [])
    Assert.Equal(Int 7, eval e3 [])
    // Assert.Equal(..., eval e4 [])

    true

[<Fact>]
let ``Exercise 6.2`` () =
    let e1 = Fun([ "x" ], Prim("*", CstI 2, Var "x"))
    let e1expected = Clos([ "x" ], Prim("*", CstI 2, Var "x"), [])

    let e2 = Let("y", CstI 22, Fun([ "z" ], Prim("+", Var "z", Var "y")))
    let e2expected = Clos([ "z" ], Prim("+", Var "z", Var "y"), [ ("y", Int 22) ])

    Assert.Equal(e1expected, eval e1 [])
    Assert.Equal(e2expected, eval e2 [])

    let e1c = Call(e1, [ CstI 2 ])
    Assert.Equal(Int 4, eval e1c [])

    let e3 = Call(Fun([ "x"; "y" ], Prim("*", Var "y", Var "x")), [ CstI 2; CstI 3 ])
    Assert.Equal(Int 6, eval e3 [])

[<Fact>]
let ``Exercise 6.3`` () =

    let e1 = fromString "let add x = fun y -> x+y in (add 2) 5 end"

    let e2 = fromString "let add = fun x -> fun y -> x+y in (add 2) 5 end"

    Assert.Equal(Int 7, eval e1 [])
    Assert.Equal(Int 7, eval e2 [])
