// micro-C example 21 -- tail call optimization is unsound

void f(int a[]) {
  print 1;
  print *(a);
  print *(a + 0);
  print *(a + 1);
  println;
}

int main() {
  int a[10];
  a[0] = 7;
  a[1] = 117;
  print 0;
  print *(a);
  print *(a + 0);
  print *(a + 1);
  println;
  f(a);


  return 0;
}
