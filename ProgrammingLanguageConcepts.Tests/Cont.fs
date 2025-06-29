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
