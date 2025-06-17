module TestsIntro1

open Xunit
open Intro1


[<Fact>]
let ``Evaluation tests`` () =
    Assert.Equal(17, eval (CstI 17))

    Assert.Equal(-1, eval (Prim("-", CstI 3, CstI 4)))

    Assert.Equal(73, eval (Prim("+", Prim("*", CstI 7, CstI 9), CstI 10)))
