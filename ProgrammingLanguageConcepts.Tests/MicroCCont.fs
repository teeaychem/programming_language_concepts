module TestsMicroCCont

open Xunit

open Machine
open MicroCParse
open MicroCSupport
open Contcomp


[<Fact>]
let ``Exercise 12.1`` () =

    let prg =
        @"
int main(int n) {
  if (n)
    { }
  else
    print 1111;
  print 2222;
}
    "

    let unopt_instr =
        List.filter
            (fun inst ->
                match inst with
                | IFZERO _ -> true
                | _ -> false)
            (cProgram (fromString prg))

    Assert.True(List.isEmpty unopt_instr)



[<Fact>]
let ``Exercise 12.2`` () =

    let prg =
        @"
int main(int n) {
  if (3 < 4)
    print 1;
  if (3 <= 4)
    print 2;
  if (3 > 4)
    print 3;
  if (3 >= 4)
    print 4;
  if (3 == 4)
    print 5;
  if (3 != 4)
    print 6;
}
    "

    // printfn "%A"(cProgram (fromString prg))

    let unopt_instr =
        List.filter
            (fun inst ->
                match inst with
                | LT -> true
                | _ -> false)
            (cProgram (fromString prg))

    Assert.True(List.isEmpty unopt_instr)


[<Fact>]
let ``Exercise 12.3`` () =

    let prg =
        @"
int main(int n) {
    print true ? 111 : 2222;
    print false ? 111 : 2222;

    int x;
    x = 3;

    print (x == 3 ? 1 : 0);
    print (x == 3 ? 0 : 1);

    print 2;
}
    "

    // printfn "%A"(cProgram (fromString prg))

    let unopt_instr =
        List.filter
            (fun inst ->
                match inst with
                | IFZERO _ -> true
                | _ -> false)
            (cProgram (fromString prg))

    Assert.Equal(2,List.length unopt_instr)

    Assert.Equal("111 2222 1 0 2", call_machine (fromString prg) [])
