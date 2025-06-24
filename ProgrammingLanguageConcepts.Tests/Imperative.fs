module TestsImperative

open Xunit
open Naive

[<Fact>]
let ``naive`` () =
    let ex1 =
        Block[Asgn("sum", CstI 0)
              For("i", CstI 0, CstI 100, Asgn("sum", Prim("+", Var "sum", Var "i")))
              Print(Var "sum")]

    let ex2 =
        Block[Asgn("i", CstI 1)
              Asgn("sum", CstI 0)

              While(
                  Prim("<", Var "sum", CstI 10000),
                  Block[Asgn( // Print(Var "sum")
                            "sum",
                            Prim("+", Var "sum", Var "i")
                        )

                        Asgn("i", Prim("+", CstI 1, Var "i"))]
              )

              Print(Var "i")
              Print(Var "sum")]


    Assert.Equal("5050", run ex1)

    Assert.Equal("10011", run ex2)
