module TestsContfun

open Xunit

open Contfun

[<Fact>]
let ``Contfun --- Examples in abstract syntax`` () =

    let ex1 = Letfun("f1", "x", Prim("+", Var "x", CstI 1), Call("f1", CstI 12))
    Assert.Equal("Result 13", sprintf "%A" (run1 ex1))
    Assert.Equal("Result 13", sprintf "%A" (run2 ex1))

    (* Factorial *)
    let ex2 =
        Letfun(
            "fac",
            "x",
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call("fac", Prim("-", Var "x", CstI 1)))),
            Call("fac", Var "n")
        )

    let fac10 = eval1 ex2 [ ("n", Int 10) ]
    Assert.Equal("Result 3628800", sprintf "%A" fac10)

    (* Example: deep recursion to check for constant-space tail recursion *)
    let exdeep =
        Letfun(
            "deep",
            "x",
            If(Prim("=", Var "x", CstI 0), CstI 1, Call("deep", Prim("-", Var "x", CstI 1))),
            Call("deep", Var "n")
        )

    let rundeep n = eval1 exdeep [ ("n", Int n) ]
    Assert.Equal("Result 1", sprintf "%A" (rundeep 100))

    (* Example: throw an exception inside expression *)
    let ex3 = Prim("*", CstI 11, Raise(Exn "outahere"))
    Assert.Equal("Abort \"outahere\"", sprintf "%A" (run1 ex3))
    Assert.Equal("Abort \"Uncaught exception: outahere\"", sprintf "%A" (run2 ex3))

    (* Example: throw an exception and catch it *)
    let ex4 =
        TryWith(Prim("*", CstI 11, Raise(Exn "Outahere")), Exn "Outahere", CstI 999)

    Assert.Equal("Abort \"Not implemented\"", sprintf "%A" (run1 ex4))
    Assert.Equal("Result 999", sprintf "%A" (run2 ex4))

    (* Example: throw an exception in a called function *)
    let ex5 =
        Letfun(
            "fac",
            "x",
            If(
                Prim("<", Var "x", CstI 0),
                Raise(Exn "negative x in fac"),
                If(Prim("<", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call("fac", Prim("-", Var "x", CstI 1))))
            ),
            Call("fac", CstI -10)
        )

    Assert.Equal("Abort \"negative x in fac\"", sprintf "%A" (run1 ex5))
    Assert.Equal("Abort \"Uncaught exception: negative x in fac\"", sprintf "%A" (run2 ex5))

    (* Example: throw an exception but don't catch it *)
    let ex6 = TryWith(Prim("*", CstI 11, Raise(Exn "Outahere")), Exn "Noway", CstI 999)
    Assert.Equal("Abort \"Not implemented\"", sprintf "%A" (run1 ex6))
    Assert.Equal("Abort \"Uncaught exception: Outahere\"", sprintf "%A" (run2 ex6))

    let ex7 =
        TryWith(Prim("*", CstI 11, Raise(Exn "Outahere")), Exn "Outahere", CstI 999)

    Assert.Equal("Abort \"Not implemented\"", sprintf "%A" (run1 ex7))
    Assert.Equal("Result 999", sprintf "%A" (run2 ex7))

[<Fact>]
let ``Exercise 11.5 -- Checked fibonacci`` () =

    let fib =
        If(
            Prim("<", Var "n", CstI 0),
            Raise(Exn "Negative argument"),
            Letfun(
                "fib2",
                "x",
                If(
                    Prim("<", Var "x", CstI 2),
                    CstI 1,
                    Prim("+", Call("fib2", Prim("-", Var "x", CstI 1)), Call("fib2", Prim("-", Var "x", CstI 2)))
                ),
                Call("fib2", Var "n")
            )
        )

    let ex = Letfun("fib", "x", fib, Call("fib", Var "n"))


    Assert.Equal("Result 13", sprintf "%A" (eval1 ex [ ("n", Int 6) ]))

    Assert.Equal("Result 1", sprintf "%A" (eval1 ex [ ("n", Int 0) ]))

    Assert.Equal("Abort \"Negative argument\"", sprintf "%A" (eval1 ex [ ("n", Int -1) ]))

    let ex =
        Letfun("fib", "x", fib, TryWith(Call("fib", Var "n"), Exn "Negative argument", CstI 1))

    Assert.Equal("Result 1", sprintf "%A" (eval2 ex [ ("n", Int 1) ]))
