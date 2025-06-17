module TestsIntro2

open System
open Xunit

open Intro2

let emptyenv: env = [] (* the empty environment *)

let env: env = [ "a", 3; "c", 78; "baf))))", 666; "b", 111 ]

[<Fact>]
let ``Env tests`` () =
    let cvalue = lookup env "c"
    Assert.Equal(cvalue, 78)

    Assert.Throws<Exception>(fun () -> lookup emptyenv "c" |> ignore)


[<Fact>]
let ``Evaluation tests`` () =
    let e1 = CstI 17
    let e2 = Prim("+", CstI 3, Var "a")
    let e3 = Prim("+", Prim("*", Var "b", CstI 9), Var "a")

    let e1v = eval e1 env
    let e2v1 = eval e2 env
    let e2v2 = eval e2 [ ("a", 314) ]
    let e3v = eval e3 env

    Assert.Equal(17, e1v)
    Assert.Equal(6, e2v1)
    Assert.Equal(317, e2v2)
    Assert.Equal(1002, e3v)


[<Fact>]
let ``1.1(i) tests`` () =
    let e_max = Prim("max", CstI 3, Var "c")
    Assert.Equal(lookup env "c", eval e_max env)

    let e_min = Prim("min", CstI 3, Var "c")
    Assert.Equal(3, eval e_min env)

    let e_eq = Prim("==", CstI 78, Var "c")
    Assert.Equal(1, eval e_eq env)

    let e_neq = Prim("==", Var "a", Var "c")
    Assert.Equal(0, eval e_neq env)


[<Fact>]
let ``1.1(ii) tests`` () =
    let abs a env =
        let value = eval a env

        let as_int =
            if 0 < value then
                value
            else
                let inverse = Prim("*", CstI -1, CstI value)
                eval inverse env

        CstI as_int

    let e1 = Prim("-", CstI 3, CstI 4)

    Assert.Equal(abs e1 env, CstI 1)



[<Fact>]
let ``1.1(iv) tests`` () =

    let if_case = CstI 1
    let else_case = CstI 0

    let a = If(Prim("==", CstI 1, CstI 1), if_case, else_case)
    Assert.Equal(eval if_case env, eval a env)

    let b = If(Prim("==", CstI 1, CstI 2), if_case, else_case)
    Assert.Equal(eval else_case env, eval b env)

[<Fact>]
let ``1.2(i-iii) tests`` () =
    let e1 = AwL.Sub(AwL.Var "v", AwL.Add(AwL.Var "w", AwL.Var "z"))
    Assert.Equal("(v - (w + z))", AwL.fmt e1)

    let e2 =
        AwL.Mul(AwL.CstI 2, AwL.Sub(AwL.Var "v", AwL.Add(AwL.Var "w", AwL.Var "z")))

    Assert.Equal("(2 * (v - (w + z)))", AwL.fmt e2)


[<Fact>]
let ``1.2(iv) tests`` () =
    let e1 = AwL.Sub(AwL.Var "v", AwL.CstI 0)
    Assert.Equal(AwL.Var "v", AwL.simplify e1)


[<Fact>]
let ``1.2(v) tests`` () =
    let e =
        AwL.Add(
            AwL.Mul(AwL.CstI 2, AwL.Pow(AwL.Var "x", AwL.CstI 3)),
            AwL.Add(AwL.Mul(AwL.CstI 8, AwL.Var "x"), AwL.CstI 4)
        )

    let expected =
        AwL.Add(AwL.Mul(AwL.CstI 6, AwL.Pow(AwL.Var "x", AwL.CstI 2)), AwL.Mul(AwL.CstI 8, AwL.Var "x"))

    let actual = AwL.simplify (AwL.diff e "x")

    Assert.Equal(AwL.simplify expected, actual)
