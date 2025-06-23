module TestsMicroC

open Xunit

open MicroCParse

let run = Interp.run


[<Fact>]
let ``Exercise 7.1`` () =
    let e1out = ref ""
    let e1p = fromFile "MicroC/ex1.c"
    let _e1 = run e1p [ 17 ] e1out
    e1out.Value <- e1out.Value.Trim()

    let e1expected = "17 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1"

    Assert.Equal(e1expected, e1out.Value)

    let e2out = ref ""
    let e2p = fromFile "MicroC/ex2.c"
    let _e2 = run e2p [] e2out
    e2out.Value <- e2out.Value.Trim()

    let e2expected = "-1 -1 1 -999 1 227 12 12 14 114 2 1 1"

    Assert.Equal(e2expected, e2out.Value)

[<Fact>]
let ``Exercise 7.2, arrsum, squares`` () =
    let src =
        @"
void arrsum(int n, int arr[], int *sump) {

  int idx; idx = 0;

  while (idx < n) {
    *sump = *sump + arr[idx];
    idx = idx + 1;
  }
}


void squares(int n, int arr[]) {

  int idx; idx = 0;

  while (idx < n) {
    arr[idx] = arr[idx] * arr[idx];
    idx = idx + 1;
  }
}


void main() {

  int arr[4]; arr[0] = 7; arr[1] = 13;
  arr[2] = 9; arr[3] = 8;

  int sum; sum = 0;

  arrsum(4, arr, &sum);

  print sum;

  sum = 0;

  squares(3, arr);
  arrsum(4, arr, &sum);

  print sum;
}
"

    let out = ref ""
    let a = fromString src
    let _ = run a [] out
    out.Value <- out.Value.Trim()

    Assert.Equal("37 307", out.Value)

[<Fact>]
let ``Exercise 7.2, histogram`` () =
    let src =
        @"
void cleararr(int n, int arr[]) { int idx; idx = 0; while (idx < n) { arr[idx] = 0; idx = idx + 1; } }

void printarr(int n, int arr[]) { int idx; idx = 0; while (idx < n) { print arr[idx]; idx = idx + 1; } }

void histogram(int n, int ns[], int max, int freq[]) {

  int idx; idx = 0;

  while (idx <= n) {
    int v; v = ns[idx];
    if (v < max) { freq[v] = freq[v] + 1; }
    idx = idx + 1;
  }
}

void main() {

  int freq[10];
  cleararr(10, freq);

  int arr[7];
  arr[0] = 1; arr[1] = 2; arr[2] = 1;
  arr[3] = 1; arr[4] = 1; arr[5] = 2;
  arr[6] = 0;

  histogram(7, arr, 3, freq);

  printarr(4, freq);

}
"

    let out = ref ""
    let a = fromString src
    let _ = run a [] out
    out.Value <- out.Value.Trim()

    Assert.Equal("1 4 2 0", out.Value)


[<Fact>]
let ``Exercise 7.3`` () =
    let src =
        @"
void main() {

  int sum; sum = 0;
  int i;

  for (i=0; i<100; i=i+1) {
    sum = sum+i;
  }
  print sum;

  sum = 0;
  for (i=0; i<10; i=i+1) {
    sum = sum+i;
  }
  print sum;
}
"

    let out = ref ""
    let a = fromString src
    let _ = run a [] out
    out.Value <- out.Value.Trim()

    Assert.Equal("4950 45", out.Value)
