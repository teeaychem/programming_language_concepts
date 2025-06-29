module TestsCont

open Xunit


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
        | x :: xs -> revc xs (fun v -> x :: acc)

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
