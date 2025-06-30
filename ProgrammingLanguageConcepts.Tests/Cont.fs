module TestsCont

open Xunit

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
