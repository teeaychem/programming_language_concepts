module TestsMicroCComp

open Xunit
open MicroCParse
open MicroCComp
open System.Diagnostics
open System.IO


let MICROC_DIR = Path.Combine(__SOURCE_DIRECTORY__, "MicroC")
let MACHINE = Path.Combine(MICROC_DIR, "machine")

let call_machine prg (args: int list) =

    let pf = Path.GetTempFileName()

    let _ = compileToFile prg pf

    let info = new ProcessStartInfo(MACHINE)

    info.Arguments <- sprintf "%s %A" pf (String.concat " " (List.map string args))

    info.RedirectStandardOutput <- true

    use proc = Process.Start info

    let out = ref (proc.StandardOutput.ReadToEnd())

    // Tidy output
    out.Value <- out.Value.Trim() // Trailing spaces

    proc.WaitForExit()

    File.Delete pf

    out.Value

[<Fact>]
let ``machine exists`` () = Assert.True(File.Exists MACHINE)


[<Fact>]
let ``machine call`` () =
    let e1p = fromFile (Path.Combine(MICROC_DIR, "ex1.c"))

    let e1r = call_machine e1p [ 10 ]
    let e1e = "10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(e1e, e1r)


[<Fact>]
let ``Exercise 8.3`` () =

    // print (countdown ^ value)
    let src =
        @"
void main() {

  int sum; sum = 0;
  int i;

  int countdown; countdown = 5;

  for (i = 1; i < 5; ++i) {
    print --countdown;
    print (sum = sum + i);
  }
}
"

    let ep = fromString src
    let er = call_machine ep []
    let ee = "4 1 3 3 2 6 1 10"

    Assert.Equal(ee, er)


    let src =
        @"
void main() {

    int arr[2];
    arr[0] = 111;
    arr[1] = 222;

    int idx; idx = 0;

    ++arr[++idx];

    print arr[0];
    print arr[1];
}
"

    let ep = fromString src

    let er = call_machine ep []
    let ee = "111 223"

    Assert.Equal(ee, er)

[<Fact>]
let ``Exercise 8.5, compilation`` () =
    let src =
        @"
void main() {

    int i; i = 0;
    int j;
    int k;

    j = (i == 0 ? ++i : 2);
    --i;

    k = (i == j ? i : 2);

    print i;
    print j;
    print k;
}
"


    let ep = fromString src
    let er = call_machine ep []


    Assert.Equal("0 1 2", er)


[<Fact>]
let ``Exercise 8.8, linear search`` () =

    let src =
        @"
void linsearch(int x, int len, int a[], int *res) {
  int i;
  for (i = 0; i < len; ++i) {
    if (a[i] == x) {
      *res = i;
      return;
    }
  }
  *res = len;
  return;
}

void main() {

  int arr[3];
  arr[0] = 3;
  arr[1] = 1;
  arr[2] = 2;

  int res;

  int x;
  for (x = 0; x < 5; ++x) {
    linsearch(x, 3, arr, &res);
    print res;
  }
}
"

    let ep = fromString src
    let er = call_machine ep []
    let ee = "3 1 2 0 3"

    Assert.Equal(ee, er)

[<Fact>]
let ``Exercise 8.8, swap`` () =

    let src =
        @"
void swap(int *x, int *y) {
  int tmp;
  tmp = *x;
  *x = *y;
  *y = tmp;
}

void main() {

  int i; i = 0;
  int j; j = 1;

  print i; print j;

  swap(&i, &j);

  print i; print j;
}
"

    let ep = fromString src
    let er = call_machine ep []
    let ee = "0 1 1 0"

    Assert.Equal(ee, er)
