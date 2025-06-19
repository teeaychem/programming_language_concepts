module TestsFun

open Xunit

open FunAbsyn
open Fun
open FunParse

[<Fact>]
let ``abstract`` () =
    let ex1 =
        Letfun("f1", [ "x" ], Prim("+", Var "x", CstI 1), Call(Var "f1", [ CstI 12 ]))

    Assert.Equal(13, run ex1)


[<Fact>]
let ``factorial`` () =

    let ex2 =
        Letfun(
            "fac",
            [ "x" ],
            If(Prim("=", Var "x", CstI 0), CstI 1, Prim("*", Var "x", Call(Var "fac", [ Prim("-", Var "x", CstI 1) ]))),
            Call(Var "fac", [ Var "n" ])
        )

    let fac10 = eval ex2 [ ("n", Int 10) ]

    Assert.Equal(3628800, fac10)


[<Fact>]
let ``deep`` () =
    let ex3 =
        Letfun(
            "deep",
            [ "x" ],
            If(Prim("=", Var "x", CstI 0), CstI 1, Call(Var "deep", [ Prim("-", Var "x", CstI 1) ])),
            Call(Var "deep", [ Var "count" ])
        )

    let rundeep n = eval ex3 [ ("count", Int n) ]

    Assert.Equal(1, rundeep 1)

[<Fact>]
let ``scope`` () =
    let ex4 =
        Let(
            "y",
            CstI 11,
            Letfun("f", [ "x" ], Prim("+", Var "x", Var "y"), Let("y", CstI 22, Call(Var "f", [ CstI 3 ])))
        )

    // dynamic scope = 25
    Assert.Equal(14, run ex4)


[<Fact>]
let ``two defs`` () =

    let ge2 b =
        Letfun("ge2", [ "x" ], Prim("<", CstI 1, Var "x"), b)

    let fib b =
        Letfun(
            "fib",
            [ "n" ],
            If(
                Call(Var "ge2", [ Var "n" ]),
                Prim(
                    "+",
                    Call(Var "fib", [ Prim("-", Var "n", CstI 1) ]),
                    Call(Var "fib", [ Prim("-", Var "n", CstI 2) ])
                ),
                CstI 1
            ),
            b
        )

    let ex5 = ge2 (fib (Call(Var "fib", [ CstI 25 ])))

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
    Assert.Equal(120, eval ex2 [ ("n", Int 5) ])

    (* Example: deep recursion to check for constant-space tail recursion *)

    let ex3 = fromString @"let deep x = if x=0 then 1 else deep(x-1) in deep count end"
    Assert.Equal(1, eval ex3 [ ("count", Int 1) ])

    (* Example: static scope (result 14) or dynamic scope (result 25) *)

    let ex4 =
        fromString
            @"let y = 11
        in let f x = x + y
          in let y = 22 in f 3 end
          end
        end"

    Assert.Equal(14, run ex4)

    (* Example: two function definitions: a comparison and Fibonacci *)

    let ex5 =
        fromString
            @"let ge2 x = 1 < x
  in let fib n = if ge2(n) then fib(n-1) + fib(n-2) else 1
    in fib 25
    end
  end"

    Assert.Equal(121393, run ex5)

[<Fact>]
let ``Exercise 4.2`` () =

    let sum n = n * (n + 1) / 2

    let x =
        Letfun(
            "sum",
            [ "n" ],
            If(Prim("=", Var "n", CstI 1), CstI 1, Prim("+", Var "n", Call(Var "sum", [ Prim("-", Var "n", CstI 1) ]))),
            Call(Var "sum", [ CstI 1000 ])
        )

    Assert.Equal(sum 1000, run x)


    let sum_expr =
        @"
    let sum n = if n = 1 then 1 else n + sum(n - 1) in sum 1000 end
    "

    Assert.Equal(sum 1000, run (fromString sum_expr))

    let pow_expr =
        @"
    let base = 3
    in let pow n = if n = 0 then 1 else base * pow(n - 1)
        in pow 8 end
    end
    "

    Assert.Equal(6561, run (fromString pow_expr))

    let pow_sum_expr b l =
        (sprintf
            "
    let base = %d in
        let limit = %d in
            let pow n = if n = 0 then 1 else base * pow(n - 1) in
                let sum n = if n = 0 then 1 else pow n + sum(n - 1) in
                    sum limit
                end
            end
        end
    end
    "
            b
            l)

    // No pow on ints?
    let pow_sum_expected b limit =
        List.fold (fun acc next -> acc + b ** next) 0.0 [ 0.0 .. limit ]

    Assert.Equal(int (pow_sum_expected 3 11.0), run (fromString (pow_sum_expr 3 11)))


    let pow_sum2_expr e l =
        (sprintf
            "
    let exp = %d in
        let limit = %d in
            let sum n = if n = 0 then 0 else
                let pow m = if m = 0 then 1 else n * pow(m - 1) in
                    pow exp + sum(n - 1)
                end in
                    sum limit
            end
        end
    end
    "
            e
            l)

    let pow_sum2_expected e limit =
        List.fold (fun acc next -> acc + next ** e) 0.0 [ 0.0 .. limit ]

    let ex = 8
    let lim = 10

    Assert.Equal(int (pow_sum2_expected ex (float lim)), run (fromString (pow_sum2_expr ex lim)))


[<Fact>]
let ``Exercise 4.4`` () =
    let e1s = "let pow x n = if n=0 then 1 else x * pow x (n-1) in pow 3 8 end"
    let e1ast = fromString e1s

    Assert.Equal(6561, run e1ast)

    let e2s =
        "
    let max2 a b = if a<b then b else a
        in let max3 a b c = max2 a (max2 b c)
            in max3 25 6 62 end
        end"

    let e2ast = fromString e2s

    Assert.Equal(62, run e2ast)

    let e3s =
        "
    n - 1"

    let e3ast = fromString e3s

    Assert.Equal(1, eval e3ast [ "n", Int 2 ])

[<Fact>]
let ``Exercise 4.5`` () =

    Assert.Equal(1, run (fromString "1 && 0 || 1"))

    Assert.Equal(0, run (fromString "1 && 0"))

    Assert.Equal(1, eval (fromString "x || y") [ "x", Int 0; "y", Int 1 ])


[<Fact>]
let ``Exercise 4.6`` () =

    Assert.Throws<System.Exception>(fun () -> fromString "( , )" |> ignore)
    |> ignore

    Assert.Throws<System.Exception>(fun () -> fromString "(1, )" |> ignore)
    |> ignore

    Assert.Throws<System.ArgumentException>(fun () -> run (fromString "#0(1, 2)") |> ignore)
    |> ignore

    Assert.Throws<System.ArgumentException>(fun () -> run (fromString "#3(1, 2)") |> ignore)
    |> ignore

    let e1 = fromString "#2(1, n - 2)"
    Assert.Equal(1, eval e1 [ "n", Int 3 ])

    let e2 = fromString "#1(3, 2, 1)"
    Assert.Equal(3, run e2)

    let e3 = fromString "let t = (1+2, false, 5<8) in if #3(t) then #1(t) else 14 end"
    Assert.Equal(3, run e3)

    let e4 = fromString "let t = (1+2, false, 8<5) in if #3(t) then #1(t) else 14 end"
    Assert.Equal(14, run e4)

    let e5 = fromString "
    let max xy = if #2(xy) < #1(xy) then #1(xy) else #2(xy) in max (3, 88) end
    "
    Assert.Equal(88, run e5)
