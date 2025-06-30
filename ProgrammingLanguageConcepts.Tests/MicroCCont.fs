module TestsMicroCCont

open Xunit

open Machine
open MicroCParse
// open MicroCSupport
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

    let zero_instrs =
        List.filter
            (fun inst ->
                match inst with
                | IFZERO _ -> true
                | _ -> false)
            (cProgram (fromString prg))

    Assert.True(List.isEmpty zero_instrs)
