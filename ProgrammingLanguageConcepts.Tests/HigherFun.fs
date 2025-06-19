module TestsHigherFun

open Xunit

[<Fact>]
let ``Exercise 5.1`` () =
    let rec merge (a: int list) (b: int list) : int list =
        match a, b with
        | [], [] -> []
        | [], b -> b
        | a, [] -> a
        | ah :: at, bh :: bt -> if ah < bh then ah :: merge at b else bh :: merge a bt

    let ecmp =
        List.forall2 (fun a b -> a = b) (merge [ 3; 5; 12 ] [ 2; 3; 4; 7 ]) [ 2; 3; 3; 4; 5; 7; 12 ]

    Assert.True ecmp
