module TestsExpr

open Xunit
open System
open System.Text.RegularExpressions

open Parse
open Expr


[<Fact>]
let ``readme`` () =
    let arun = run (fromString "2 + 3 * 4")
    Assert.Equal(14, arun)

    let ev1 = eval (fromString "2 + x * 4") [ ("x", 3) ]
    Assert.Equal(14, ev1)

    let ev2 = eval (fromString "let x = 1+2 in 2 + x * 4 end") []
    Assert.Equal(14, ev2)

    let code1 = scomp (fromString "2 + 3 * 4") []
    Assert.Equal(14, seval code1 [])

    let code2 = scomp (fromString "2 + x * 4") [ Bound "x" ]
    Assert.Equal(14, seval code2 [ 3 ])

    let code3 = scomp (fromString "let x = 1+2 in 2 + x * 4 end") []
    Assert.Equal(14, seval code3 [])



[<Fact>]
let ``Exercise 3.2`` () =
    let rx = Regex(@"^a$|^b$|^(?:(?:a?b+)+)a$", RegexOptions.Compiled)

    Assert.True(rx.IsMatch "b")
    Assert.True(rx.IsMatch "a")
    Assert.True(rx.IsMatch "ba")
    Assert.True(rx.IsMatch "ababbbaba")

    Assert.False(rx.IsMatch "aa")
    Assert.False(rx.IsMatch "babaa")


[<Fact>]
let ``Exercise 3.5`` () =

    let fs0 = fromString "1 + 2 * 3"
    let fs0r = run fs0
    Assert.Equal(7, fs0r)

    let fs1 = fromString "1 - 2 - 3"
    let fs1r = run fs1
    Assert.Equal(-4, fs1r)

    let fs2 = fromString "1 + -2"
    let fs2r = run fs2
    Assert.Equal(-1, fs2r)

    Assert.Throws<Exception>(fun () -> fromString "x++" |> ignore) |> ignore

    Assert.Throws<Exception>(fun () -> fromString "1 + 1.2" |> ignore) |> ignore

    Assert.Throws<Exception>(fun () -> fromString "1 + " |> ignore) |> ignore

    let fs6 = fromString "let z = (17) in z + 2 * 3 end"
    let fs6r = run fs6
    Assert.Equal(23, fs6r)

    Assert.Throws<Exception>(fun () -> fromString "let z = 17) in z + 2 * 3 end" |> ignore)
    |> ignore

    Assert.Throws<Exception>(fun () -> fromString "let in = (17) in z + 2 * 3 end" |> ignore)
    |> ignore

    let fs9 = fromString "1 + let x=5 in let y=7+x in y+y end + x end"
    let fs9r = run fs9
    Assert.Equal(30, fs9r)


[<Fact>]
let ``Exercise 3.6`` () =
    let compString (str: string) : sinstr list =
        let expr = fromString str
        scomp expr []

    Assert.Equal(7, seval (compString "1 + 2 * 3") [])


[<Fact>]
let ``Exercise 3.7`` () =

    // As there's no equality, a condition is true whenever the expression evaluates to a non-zero value and false otherwise.

    let ok = "if 1 then 2 * 3 else 3 - 4"
    let okexpr = fromString ok
    Assert.Equal(fmt okexpr, ok)
    Assert.Equal(6, eval okexpr [])

    let nok = "if 0 then 2 * 3 else 3 - 4"
    let okexpr = fromString nok
    Assert.Equal(fmt okexpr, nok)
    Assert.Equal(-1, eval okexpr [])

    true
