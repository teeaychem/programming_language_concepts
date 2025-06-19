module TestsFun

open Xunit

open FunAbsyn
open Fun
open FunParse

[<Fact>]
let ``abstract`` () =
    let ex1 = Letfun("f1", "x", Prim("+", Var "x", CstI 1), Call(Var "f1", CstI 12))

    Assert.Equal(13, run ex1)


[<Fact>]
let ``factorial`` () =

    let ex2 =
        Letfun(
            "fac",
            "x",
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call(Var "fac", Prim("-", Var "x", CstI 1)))),
            Call(Var "fac", Var "n")
        )

    let fac10 = eval ex2 [ ("n", Int 10) ]

    Assert.Equal(3628800, fac10)


[<Fact>]
let ``deep`` () =
    let ex3 =
        Letfun(
            "deep",
            "x",
            If(Prim("=", Var "x", CstI 0), CstI 1, Call(Var "deep", Prim("-", Var "x", CstI 1))),
            Call(Var "deep", Var "count")
        )

    let rundeep n = eval ex3 [ ("count", Int n) ]

    Assert.Equal(1, rundeep 1)

[<Fact>]
let ``scope`` () =
    let ex4 =
        Let("y", CstI 11, Letfun("f", "x", Prim("+", Var "x", Var "y"), Let("y", CstI 22, Call(Var "f", CstI 3))))

    // dynamic scope = 25
    Assert.Equal(14, run ex4)


[<Fact>]
let ``two defs`` () =

    let ge2 b =
        Letfun("ge2", "x", Prim("<", CstI 1, Var "x"), b)

    let fib b =
        Letfun(
            "fib",
            "n",
            If(
                Call(Var "ge2", Var "n"),
                Prim("+", Call(Var "fib", Prim("-", Var "n", CstI 1)), Call(Var "fib", Prim("-", Var "n", CstI 2))),
                CstI 1
            ),
            b
        )

    let ex5 = ge2 (fib (Call(Var "fib", CstI 25)))

    Assert.Equal(121393, run ex5)

[<Fact>]
let ``parse`` () =
    let e1 = fromString "5+7"
    let e2 = fromString "let y = 7 in y + 2 end"
    let e3 = fromString "let f x = x + 7 in f 2 end"

    Assert.Equal(12, run e1)
    Assert.Equal(9, run e2)
    Assert.Equal(9, run e3)

[<Fact>]
let ``additional parse`` () =
    (* Examples in concrete syntax *)

    let ex1 = fromString @"let f1 x = x + 1 in f1 12 end"
    Assert.Equal(13, run ex1)

    (* Example: factorial *)

    let ex2 = fromString @"let fac x = if x=0 then 1 else x * fac(x - 1) in fac n end"
    Assert.Equal(120, eval ex2 [("n", Int 5)])

    (* Example: deep recursion to check for constant-space tail recursion *)

    let ex3 = fromString @"let deep x = if x=0 then 1 else deep(x-1) in deep count end"
    Assert.Equal(1, eval ex3 [("count", Int 1)])

    (* Example: static scope (result 14) or dynamic scope (result 25) *)

    let ex4 = fromString @"let y = 11
        in let f x = x + y
          in let y = 22 in f 3 end
          end
        end"
    Assert.Equal(14, run ex4)

    (* Example: two function definitions: a comparison and Fibonacci *)

    let ex5 = fromString @"let ge2 x = 1 < x
  in let fib n = if ge2(n) then fib(n-1) + fib(n-2) else 1
    in fib 25
    end
  end"
    Assert.Equal(121393, run ex5)
