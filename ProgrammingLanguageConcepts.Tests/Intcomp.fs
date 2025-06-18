module TestsIntcomp

open Xunit
open Intcomp1

let EqualLists a b =
    List.forall (fun (ea, eb) -> ea = eb) (List.zip a b)

let emptyenv: eval_env = []

let e1 = Let("z", CstI 17, Prim("+", Var "z", Var "z"))

let e2 =
    Let("z", CstI 17, Prim("+", Let("z", CstI 22, Prim("*", CstI 100, Var "z")), Var "z"))

let e3 = Let("z", Prim("-", CstI 5, CstI 4), Prim("*", CstI 100, Var "z"))

let e4 =
    Prim("+", Prim("+", CstI 20, Let("z", CstI 17, Prim("+", Var "z", CstI 2))), CstI 30)

let e5 = Prim("*", CstI 2, Let("x", CstI 3, Prim("+", Var "x", CstI 4)))

let e6 = Prim("+", Var "y", Var "z")

let e7 = Prim("+", Let("z", CstI 22, Prim("*", CstI 5, Var "z")), Var "z")

let e8 = Let("z", Prim("*", CstI 22, Var "z"), Prim("*", CstI 5, Var "z"))

let e9 = Let("z", CstI 22, Prim("*", Var "y", Var "z"))

[<Fact>]
let ``Closed expression tests`` () =

    Assert.Equal(34, eval e1 emptyenv)

    Assert.Equal(2217, eval e2 emptyenv)

    Assert.Equal(100, eval e3 emptyenv)

    Assert.Equal(69, eval e4 emptyenv)

    Assert.Equal(14, eval e5 emptyenv)


[<Fact>]
let ``Substitution tests`` () =

    let e6s1 = nsubst e6 [ ("z", CstI 17) ]
    let e6s1_expected = Prim("+", Var "y", CstI 17)
    Assert.Equal(e6s1_expected, e6s1)

    let e6s2 = nsubst e6 [ ("z", Prim("-", CstI 5, CstI 4)) ]
    let e6s2_expected = Prim("+", Var "y", Prim("-", CstI 5, CstI 4))
    Assert.Equal(e6s2_expected, e6s2)

    let e6s3 = nsubst e6 [ ("z", Prim("+", Var "z", Var "z")) ]
    let e6s3_expected = Prim("+", Var "y", Prim("+", Var "z", Var "z"))
    Assert.Equal(e6s3_expected, e6s3)

    // Shows that only z outside the Let gets substituted:
    let e7s1 = nsubst e7 [ ("z", CstI 100) ]

    let e7s1_expected =
        Prim("+", Let("z", CstI 22, Prim("*", CstI 5, Var "z")), CstI 100)

    Assert.Equal(e7s1_expected, e7s1)


    // Shows that only the z in the Let rhs gets substituted
    let e8s1 = nsubst e8 [ ("z", CstI 100) ]

    let e8s1_expected =
        Let("z", Prim("*", CstI 22, CstI 100), Prim("*", CstI 5, Var "z"))

    Assert.Equal(e8s1_expected, e8s1)

    // Shows (wrong) capture of free variable z under the let:
    let e9s1 = nsubst e9 [ ("y", Var "z") ]
    let e9s1_expected = Let("z", CstI 22, Prim("*", Var "z", Var "z"))
    Assert.Equal(e9s1_expected, e9s1)

    let e9s2 = nsubst e9 [ ("z", Prim("-", CstI 5, CstI 4)) ]
    let e9s2_expected = Let("z", CstI 22, Prim("*", Var "y", Var "z"))
    Assert.Equal(e9s2_expected, e9s2)


[<Fact>]
let ``Capture avoiding substitution tests`` () =

    let e6s1a = subst e6 [ ("z", CstI 17) ]
    let e6s1a_expected = Prim("+", Var "y", CstI 17)
    Assert.Equal(e6s1a_expected, e6s1a)

    let e6s2a = subst e6 [ ("z", Prim("-", CstI 5, CstI 4)) ]
    let e6s2a_expected = Prim("+", Var "y", Prim("-", CstI 5, CstI 4))
    Assert.Equal(e6s2a_expected, e6s2a)

    let e6s3a = subst e6 [ ("z", Prim("+", Var "z", Var "z")) ]
    let e6s3a_expected = Prim("+", Var "y", Prim("+", Var "z", Var "z"))
    Assert.Equal(e6s3a_expected, e6s3a)

    // Shows renaming of bound variable z (to z1)
    let e7s1a = subst e7 [ ("z", CstI 100) ]

    let e7s1a_expected =
        Prim("+", Let("z1", CstI 22, Prim("*", CstI 5, Var "z1")), CstI 100)

    Assert.Equal(e7s1a_expected, e7s1a)

    // Shows renaming of bound variable z (to z2)
    let e8s1a = subst e8 [ ("z", CstI 100) ]

    let e8s1a_expected =
        Let("z2", Prim("*", CstI 22, CstI 100), Prim("*", CstI 5, Var "z2"))

    Assert.Equal(e8s1a_expected, e8s1a)

    // Shows renaming of bound variable z (to z3), avoiding capture of free z
    let e9s1a = subst e9 [ ("y", Var "z") ]
    let e9s1a_expected = Let("z3", CstI 22, Prim("*", Var "z", Var "z3"))
    Assert.Equal(e9s1a_expected, e9s1a)


[<Fact>]
let ``Compilation`` () =

    let s1 = scomp e1 []
    let s1_expected = [ SCstI 17; SVar 0; SVar 1; SAdd; SSwap; SPop ]
    Assert.True(EqualLists s1 s1_expected)

    let s2 = scomp e2 []

    let s2_expected =
        [ SCstI 17
          SCstI 22
          SCstI 100
          SVar 1
          SMul
          SSwap
          SPop
          SVar 1
          SAdd
          SSwap
          SPop ]

    Assert.True(EqualLists s2 s2_expected)

    let s3 = scomp e3 []
    let s3_expected = [ SCstI 5; SCstI 4; SSub; SCstI 100; SVar 1; SMul; SSwap; SPop ]
    Assert.True(EqualLists s3 s3_expected)

    let s5 = scomp e5 []
    let s5_expected = [ SCstI 2; SCstI 3; SVar 0; SCstI 4; SAdd; SSwap; SPop; SMul ]
    Assert.True(EqualLists s5 s5_expected)

[<Fact>]
let ``Stack evaluation`` () =
    let rpn1 = reval [ RCstI 10; RCstI 17; RDup; RMul; RAdd ] []

    Assert.Equal(299, rpn1)



[<Fact>]
let ``Exercise 2.1`` () =
    let e1 =
        Lets([ "x1", Prim("+", CstI 5, CstI 7); "x2", Prim("*", Var "x1", CstI 2) ], Prim("+", Var "x1", Var "x2"))

    Assert.Equal(36, eval e1 emptyenv)

[<Fact>]
let ``Exercise 2.2`` () =
    let e1 =
        Lets([ "x1", Prim("+", CstI 5, CstI 7); "x2", Prim("*", Var "x1", CstI 2) ], Prim("+", Var "x3", Var "x2"))

    Assert.True(EqualLists [ "x3" ] (freevars e1))

    let e2 = Lets([ "x1", Prim("+", Var "x1", CstI 7) ], Prim("+", Var "x1", CstI 8))

    Assert.True(EqualLists [ "x1" ] (freevars e2))

[<Fact>]
let ``Exercise 2.3`` () =
    let cenv: string list = []

    let e1_expected = TLet(TCstI 17, TPrim("+", TVar 0, TVar 0))

    Assert.Equal(e1_expected, tcomp e1 cenv)

    let e2 =
        Lets([ "x1", Prim("+", CstI 5, CstI 7); "x2", Prim("*", Var "x1", CstI 2) ], Prim("+", Var "x1", Var "x2"))

    let e2_expected =
        TLets([ TPrim("+", TCstI 5, TCstI 7); TPrim("*", TVar 0, TCstI 2) ], TPrim("+", TVar 1, TVar 0))

    Assert.Equal(e2_expected, tcomp e2 cenv)

    Assert.Equal(eval e2 [], teval (tcomp e2 []) [])

[<Fact>]
let ``Exercises 2.4 & 2.5`` () =

    let cenv = []

    let _file_path = intsToFile (assemble (scomp e1 cenv)) "assembly.txt"

    // The rest is up to you.

    Assert.True true

[<Fact>]
let ``Exercise 2.6`` () =

    // eval is rebound here for the modification.
    let rec eval e (env: eval_env) : int =
        match e with
        | CstI i -> i
        | Var x -> lookup env x
        | Let(x_var, x_val, ebody) ->
            let x_env = (x_var, eval x_val env) :: env
            eval ebody x_env

        | Lets(evals, ebody) ->

            // Pair vars with vals evaluated wrt. env
            let together =
                List.fold (fun acc (xvar, xval) -> (xvar, eval xval env) :: acc) [] evals

            // Bind each var with evaluated val wrt. env
            let renv = List.fold (fun env (xvar, xval) -> (xvar, xval) :: env) env together

            eval ebody renv

        | Prim("+", e1, e2) -> eval e1 env + eval e2 env
        | Prim("*", e1, e2) -> eval e1 env * eval e2 env
        | Prim("-", e1, e2) -> eval e1 env - eval e2 env
        | Prim _ -> failwith "unknown primitive"

    let e1 =
        Let("x", CstI 11, Lets([ "x", CstI 22; "y", Prim("+", Var "x", CstI 1) ], Prim("+", Var "x", Var "y")))

    let e1v = eval e1 []

    Assert.Equal(34, e1v)
