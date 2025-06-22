module TestsHigherFun


open Xunit

open FunParse
open FunAbsyn
open HigherFun
open TypeInference

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
let ``parse and type`` () =

    let tex1 = inferType (fromString "let f x = 1 in f 7 + f false end")
    Assert.Equal("int", tex1)

    let tex2 =
        inferType (fromString "let g = let f x = 1 in f end in g 7 + g false end")

    Assert.Equal("int", tex2)

    let tex3 =
        inferType (fromString "let g y = let f x = (x=y) in f 1 = f 3 end in g 7 end")

    Assert.Equal("bool", tex3)

    let tex4 =
        inferType (
            fromString
                @"let tw g = let app x = g (g x) in app end
                     in let triple y = 3 * y in (tw triple) 11 end
                     end"
        )

    Assert.Equal("int", tex4)

    let tex5 =
        inferType (
            fromString
                @"let tw g = let app x = g (g x) in app end
                     in tw end"
        )

    Assert.Equal("( ( 'h -> 'h ) -> ( 'h -> 'h ) )", tex5)

    // (* Declaring a polymorphic function and rebinding it *)

    let tex6 =
        inferType (
            fromString
                @"let id x = x
                     in let i1 = id
                     in let i2 = id
                     in let k x = let k2 y = x in k2 end
                     in (k 2) (i1 false) = (k 4) (i1 i2) end end end end "
        )

    Assert.Equal("bool", tex6)

    let tex8 = inferType (fromString "let f x = x in f f end")
    Assert.Equal("( 'e -> 'e )", tex8)


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

[<Fact>]
let ``Exercise 6.5(1)`` () =
    let e1s = "let f x = 1 in f f end"
    let e1 = fromString e1s

    let e2s = "let f g = g g in f end"
    let e2 = fromString e2s

    let e3s = "let f x = let g y = y in g false end in f 42 end"
    let e3 = fromString e3s

    let e3xs = "let f x = let g y = y in g 1 end in f true end"
    let e3x = fromString e3xs

    let e4s = "let f x = let g y = if true then y else x in g false end in f 42 end"
    let e4 = fromString e4s

    let e5s = "let f x = let g y = if true then y else x in g false end in f true end"
    let e5 = fromString e5s

    let em1s = "let add x y = x + y in add 3 4 end"
    let em1 = fromString em1s

    let em2s = "let add x y = x + y in add 3 false end"
    let em2 = fromString em2s

    Assert.Equal("int", inferType e1)

    let e2em = Assert.Throws<System.Exception>(fun () -> inferType e2 |> ignore)
    Assert.Equal("type error: circularity", e2em.Message)

    Assert.Equal("bool", inferType e3)
    Assert.Equal("int", inferType e3x)

    let e4em = Assert.Throws<System.Exception>(fun () -> inferType e4 |> ignore)
    Assert.Equal("type error: bool and int", e4em.Message)

    Assert.Equal("bool", inferType e5)

    Assert.Equal("int", inferType em1)

    let em2em = Assert.Throws<System.Exception>(fun () -> inferType em2 |> ignore)
    Assert.Equal("type error: int and bool", em2em.Message)

[<Fact>]
let ``Exercise 6.5(2)`` () =

    let bbb = Fun([ "x"; "y" ], If(Var "x", Var "x", Var "y"))
    Assert.Equal("( bool bool -> bool )", inferType bbb)

    let bi = Fun([ "x" ], If(Var "x", CstI 1, CstI 0))
    Assert.Equal("( bool -> int )", inferType bi)

    let a1 = Fun([ "x" ], Var "x")
    Assert.Equal("( 'b -> 'b )", inferType a1)

    let a2 = Fun([ "x"; "y" ], Var "x")
    Assert.Equal("( 'b 'c -> 'b )", inferType a2)

    let a3 = Fun([ "x"; "y" ], Prim("=", Var "y", Var "x"))
    Assert.Equal("( 'c 'c -> bool )", inferType a3)

    let a4 = Fun([ "x" ], Fun([ "y" ], Var "x"))
    Assert.Equal("( 'b -> ( 'c -> 'b ) )", inferType a4)
