// micro-C example 2

void main() {
  int *p;         // pointer to int
  int i;          // int
  int ia[10];     // array of 10 ints
  int *ia2;       // pointer to int
  int *ipa[10];   // array of 10 pointers to int
  int (*iap)[10]; // pointer to array of 10 ints

  print i; // [0] ~1

  p = &i;         // now p points to i
  print p;        // [1] ~1
  print(p == &i); // [2] 1

  ia2 = ia;   // now ia2 points to ia[0]
  print *ia2; // [3] ~1

  *p = 227; // now i is 227
  print p;  // [4] 1
  print i;  // [5] 227

  *&i = 12; // now i is 12
  print i;  // [6] 12

  p = &*p;  // no change
  print *p; // [7] 12

  p = ia;   // now p points to ia[0]
  print p;  // [8]
  print ia; // [9]

  *ia = 14;    // now ia[0] is 14
  print ia[0]; // [10] 14

  *(ia + 9) = 114; // now ia[9] is 114
  print ia[9];     // [11] 114

  ipa[2] = p;    // now ipa[2] points to i
  print p;       // [12] 2
  print ipa[2];  // [13] 2
  print *ipa[2]; // [14] 2

  print(&*ipa[2] == &**(ipa + 2)); // [15] 1 (true)

  print(&(*iap)[2] == &*((*iap) + 2)); // [16] 1 (true)
}
