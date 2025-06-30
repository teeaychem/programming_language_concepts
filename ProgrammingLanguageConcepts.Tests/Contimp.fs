module TestsContimp

open Xunit
open Contimp

[<Fact>]
let ``Contimp --- Example programs`` () =

    (* Abruptly terminating a for loop *)
    let ex1 =
        For("i", CstI 0, CstI 10, If(Prim("==", Var "i", CstI 7), Throw(Exn "seven"), Print(Var "i")))

    let ex1o = ref ""
    Assert.Equal("Abort \"Uncaught exception: seven\"", sprintf "%A" (run1 ex1 ex1o))
    Assert.Equal("0\n1\n2\n3\n4\n5\n6\n", ex1o.Value)

    let ex1o = ref ""
    Assert.Equal("Abort \"Uncaught exception: seven\"", sprintf "%A" (run2 ex1 ex1o))
    Assert.Equal("0\n1\n2\n3\n4\n5\n6\n", ex1o.Value)

    (* Abruptly terminating a while loop *)
    let ex2 =
        Block[Asgn("i", CstI 0)

              While(
                  CstI 1,
                  Block[Asgn("i", Prim("+", Var "i", CstI 1))
                        Print(Var "i")
                        If(Prim("==", Var "i", CstI 7), Throw(Exn "seven"), Block [])]
              )

              Print(CstI 333333)]

    let ex2o = ref ""
    Assert.Equal("Abort \"Uncaught exception: seven\"", sprintf "%A" (run1 ex2 ex2o))
    Assert.Equal("1\n2\n3\n4\n5\n6\n7\n", ex2o.Value)

    let ex2o = ref ""
    Assert.Equal("Abort \"Uncaught exception: seven\"", sprintf "%A" (run2 ex2 ex2o))
    Assert.Equal("1\n2\n3\n4\n5\n6\n7\n", ex2o.Value)

    (* Abruptly terminating a while loop, and handling the exception *)
    let ex3 =
        Block[Asgn("i", CstI 0)

              TryCatch(
                  Block[While(
                            CstI 1,
                            Block[Asgn("i", Prim("+", Var "i", CstI 1))
                                  Print(Var "i")
                                  If(Prim("==", Var "i", CstI 7), Throw(Exn "seven"), Block [])]
                        )

                        Print(CstI 111111)],
                  Exn "seven",
                  Print(CstI 222222)
              )

              Print(CstI 333333)]

    let ex3o = ref ""
    Assert.Equal("Abort \"TryCatch is not implemented\"", sprintf "%A" (run1 ex3 ex3o))
    Assert.Equal("", ex3o.Value)

    let ex3o = ref ""
    Assert.Equal("Terminate", sprintf "%A" (run2 ex3 ex3o))
    Assert.Equal("1\n2\n3\n4\n5\n6\n7\n222222\n333333\n", ex3o.Value)


[<Fact>]
let ``Exercise 11.1`` () =

    let rec lenc xs k =
        match xs with
        | [] -> k 0
        | _ :: xs -> lenc xs (fun v -> k (1 + v))

    Assert.Equal("The answer is ’3’", lenc [ 2; 5; 7 ] (sprintf "The answer is ’%d’"))

    Assert.Equal(6, lenc [ 2; 5; 7 ] (fun v -> 2 * v))


    let rec leni xs acc =
        match xs with
        | [] -> acc
        | _ :: xs -> leni xs 1 + acc

    Assert.Equal(3, leni [ 2; 5; 7 ] 0)
