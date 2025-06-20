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

[<Fact>]
let ``Exercise 5.3`` () =
    let rec mergep (a: 'a list) (b: 'a list) (cmp: 'a * 'a -> int) : 'a list =
        match a, b with
        | [], [] -> []
        | [], b -> b
        | a, [] -> a
        | ah :: at, bh :: bt ->
            if cmp (ah, bh) < 0 then
                ah :: mergep at b cmp
            else
                bh :: mergep a bt cmp

    let icmp (x, y) =
        if x < y then -1
        else if x > y then 1
        else 0


    let int_cmp =
        List.forall2 (fun a b -> a = b) (mergep [ 3; 5; 12 ] [ 2; 3; 4; 7 ] icmp) [ 2; 3; 3; 4; 5; 7; 12 ]

    Assert.True int_cmp

    let ss1 = [ "abc"; "apricot"; "ballad"; "zebra" ]
    let ss2 = [ "abelian"; "ape"; "carbon"; "yosemite" ]

    let str_cmp =
        List.forall2
            (fun a b -> a = b)
            (mergep ss1 ss2 icmp)
            [ "abc"; "abelian"; "ape"; "apricot"; "ballad"; "carbon"; "yosemite"; "zebra" ]

    Assert.True str_cmp

    let ps1 = [ 10, 4; 10, 7; 12, 0; 12, 1 ]
    let ps2 = [ 9, 100; 10, 5; 12, 2; 13, 0 ]

    let pcmp ((x1, x2), (y1, y2)) =
        if x1 < y1 then -1
        else if x1 = y1 then icmp (x2, y2)
        else 1

    let p_cmp =
        List.forall2
            (fun a b -> a = b)
            (mergep ps1 ps2 pcmp)
            [ 9, 100; 10, 4; 10, 5; 10, 7; 12, 0; 12, 1; 12, 2; 13, 0 ]

    Assert.True p_cmp
