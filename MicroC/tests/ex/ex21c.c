// micro-C example 21 -- tail call optimization is unsound

void f(int a[]) {
  print a[0];
  print a[1];
}

int main() {
  int a[10];
  a[0] = 7;
  a[1] = 117;
  f(a);
  print a[0];
  print a[1];

  return 0;
}
