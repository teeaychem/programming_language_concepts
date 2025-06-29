module TestsCont

open Xunit
open Icon


[<Fact>]
let ``Icon`` () =

    // (write(1 to 3)) ; fail
    let ex1 = Seq(Write(FromTo(1, 3)), Fail)
    let ex1out = ref ""
    let _ = run ex1 ex1out
    Assert.Equal("1 Failed", ex1out.Value) // Don't backtrack

    // (write(1 to 3)) & fail
    let ex2 = And(Write(FromTo(1, 3)), Fail)
    let ex2out = ref ""
    let _ = run ex2 ex2out
    Assert.Equal("1 2 3 Failed", ex2out.Value) // Cartesian

    // (write((1 to 3) & (4 to 6))) & fail
    let ex3and = And(Write(And(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3andout = ref ""
    let _ = run ex3and ex3andout
    Assert.Equal("4 5 6 4 5 6 4 5 6 Failed", ex3andout.Value)

    // (write((1 to 3) | (4 to 6))) & fail
    let ex3or = And(Write(Or(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3orout = ref ""
    let _ = run ex3or ex3orout
    Assert.Equal("1 2 3 4 5 6 Failed", ex3orout.Value) // Union

    // (write((1 to 3) ; (4 to 6))) & fail
    let ex3seq = And(Write(Seq(FromTo(1, 3), FromTo(4, 6))), Fail)
    let ex3seqout = ref ""
    let _ = run ex3seq ex3seqout
    Assert.Equal("4 5 6 Failed", ex3seqout.Value)

    // write((1 to 2) & ((4 to 5) & "found"))
    let ex4 = Write(And(FromTo(1, 2), And(FromTo(4, 5), CstS "found")))
    let ex4out = ref ""
    let _ = run ex4 ex4out
    Assert.Equal("found ", ex4out.Value) // No failure to backtrack from


    // write((1 to 3) & ((4 to 6) & "found"))
    let ex4x = Write(And(ex4, Fail))
    let ex4xout = ref ""
    let _ = run ex4x ex4xout
    Assert.Equal("found found found found Failed", ex4xout.Value) // No failure to backtrack from

    // every(write(1 to 3))
    let ex5 = Every(Write(FromTo(1, 3)))
    let ex5out = ref ""
    let _ = run ex5 ex5out
    Assert.Equal("1 2 3 ", ex5out.Value) // Explore every path

    // (every(write(1 to 3)) & (4 to 6))
    let ex6 = And(Every(Write(FromTo(1, 3))), FromTo(4, 6))
    let ex6out = ref ""
    let _ = run ex6 ex6out
    Assert.Equal("1 2 3 ", ex6out.Value)

    // every(write((1 to 3) + (4 to 6)))
    let ex7 = Every(Write(Prim("+", FromTo(1, 3), FromTo(4, 6))))
    let ex7out = ref ""
    let _ = run ex7 ex7out
    Assert.Equal("5 6 7 6 7 8 7 8 9 ", ex7out.Value)

    // write(4 < (1 to 10))
    let ex8 = Write(Prim("<", CstI 4, FromTo(1, 10)))
    let ex8out = ref ""
    let _ = run ex8 ex8out
    Assert.Equal("5 ", ex8out.Value)

    true


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


[<Fact>]
let ``Exercise 11.2`` () =

    let rec revc (xs: 'a list) (k: 'a list -> 'a list) : 'a list =
        match xs with
        | [] -> k []
        | x :: xs -> revc xs (fun v -> x :: k v)

    let result = sprintf "%A" (revc [ 2; 5; 7 ] id)
    Assert.Equal("[7; 5; 2]", result)

    let result = sprintf "%A" (revc [ 2; 5; 7 ] (fun v -> v @ v))
    Assert.Equal("[7; 5; 2]", result)


    let rec revi (xs: 'a list) acc : 'a list =
        match xs with
        | [] -> acc
        | x :: xs -> revc xs (fun _ -> x :: acc)

    let result = sprintf "%A" (revi [ 2; 5; 7 ] [])
    Assert.Equal("[7; 5; 2]", result)



[<Fact>]
let ``Exercise 11.3`` () =

    let rec prodc xs k =
        match xs with
        | [] -> k 1
        | x :: xs -> prodc xs (fun v -> k (x * v))

    Assert.Equal("The answer is ’70’", prodc [ 2; 5; 7 ] (sprintf "The answer is ’%d’"))

    Assert.Equal(70, prodc [ 2; 5; 7 ] id)


[<Fact>]
let ``Exercise 11.4`` () =

    let rec prodc xs k =
        match xs with
        | [] -> k 1
        | x :: xs ->
            match x with
            | 0 -> k 0
            | 1 -> prodc xs (fun v -> k v)
            | _ -> prodc xs (fun v -> k (x * v))

    Assert.Equal(70, prodc [ 2; 5; 7 ] id)

    let rec prodi xs acc =
        match xs with
        | [] -> acc
        | x :: xs ->
            match x with
            | 0 -> 0
            | 1 -> prodi xs acc
            | _ -> prodi xs x * acc

    Assert.Equal(70, prodi [ 2; 5; 7 ] 1)
