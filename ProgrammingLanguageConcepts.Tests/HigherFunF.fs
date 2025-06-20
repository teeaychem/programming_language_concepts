module TestsHigherFunF

open Xunit

let EqualLists a b =
    List.forall (fun (ea, eb) -> ea = eb) (List.zip a b)

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

[<Fact>]
let ``Exercise 5.4`` () =
    // Test helpers
    let isEven n = n % 2 = 0
    let isOdd n = n % 2 <> 0


    // From the appendix
    let rec foldr f xs e =
        match xs with
        | [] -> e
        | x :: xr -> f x (foldr f xr e)


    let filter (p: 'a -> bool) (l: 'a list) : 'a list =
        foldr (fun e t -> if p e then e :: t else t) l []

    Assert.True(EqualLists (List.filter isEven [ 1..10 ]) (filter isEven [ 1..10 ]))

    let forall (p: 'a -> bool) (l: 'a list) : bool =
        foldr
            (fun e t ->
                if not t then false
                else if p e then true
                else false)
            l
            true

    Assert.Equal(List.forall isEven [ 1..10 ], forall isEven [ 1..10 ])
    Assert.Equal(List.forall isEven (List.filter isEven [ 1..10 ]), forall isEven (filter isEven [ 1..10 ]))

    let exists (p: 'a -> bool) (l: 'a list) : bool =
        foldr
            (fun e t ->
                if t then true
                else if p e then true
                else false)
            l
            false

    Assert.Equal(List.exists isEven [ 1..10 ], exists isEven [ 1..10 ])
    Assert.Equal(List.exists isOdd (List.filter isEven [ 1..10 ]), exists isOdd (filter isEven [ 1..10 ]))

    let mapPartial (p: 'a -> 'b option) (l: 'a list) : 'b list =
        foldr
            (fun e t ->
                match p e with
                | Some y -> y :: t
                | None -> t)
            l
            []

    let mp =
        mapPartial (fun i -> if i > 7 then Some(i - 7) else None) [ 4; 12; 3; 17; 10 ]

    Assert.True(EqualLists mp [ 5; 10; 3 ])

type 'a tree =
    | Lf
    | Br of 'a * 'a tree * 'a tree


let rec treeFold f t e =
    match t with
    | Lf -> e
    | Br(v, t1, t2) -> f (v, treeFold f t1 e, treeFold f t2 e)

[<Fact>]
let ``Exercise 5.5`` () =

    let treeCount t =
        treeFold (fun (_, lt, rt) -> 1 + lt + rt) t 0

    let treeSum t =
        treeFold (fun (v, lt, rt) -> v + lt + rt) t 0

    let treeDepth t =
        treeFold (fun (_, lt, rt) -> 1 + max lt rt) t 0

    let treePreOrder t =
        treeFold (fun (v, lt, rt) -> v :: lt @ rt) t []

    let treeInOrder t =
        treeFold (fun (v, lt, rt) -> lt @ [ v ] @ rt) t []

    let treePostOrder t =
        treeFold (fun (v, lt, rt) -> lt @ rt @ [ v ]) t []

    let mapTree f t =
        treeFold (fun (v, lt, rt) -> Br(f v, lt, rt)) t Lf

    let t1 = Br(34, Br(23, Lf, Lf), Br(54, Lf, Br(78, Lf, Lf)))

    Assert.Equal(4, treeCount t1)

    Assert.Equal(189, treeSum t1)

    Assert.Equal(3, treeDepth t1)
    Assert.Equal(0, treeDepth Lf)

    let t2 =
        Br(1, Br(2, Br(3, Lf, Lf), Br(4, Lf, Lf)), Br(5, Br(6, Lf, Lf), Br(7, Lf, Lf)))

    let t2pre = [ 1; 2; 3; 4; 5; 6; 7 ]
    let t2in = [ 3; 2; 4; 1; 6; 5; 7 ]
    let t2post = [ 3; 4; 2; 6; 7; 5; 1 ]

    Assert.True(EqualLists t2pre (treePreOrder t2))
    Assert.True(EqualLists t2in (treeInOrder t2))
    Assert.True(EqualLists t2post (treePostOrder t2))

    Assert.True(EqualLists (List.map (fun n -> n + 1) t2pre) (treePreOrder (mapTree (fun n -> n + 1) t2)))


