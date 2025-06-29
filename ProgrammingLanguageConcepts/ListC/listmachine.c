/* File ListC/listmachine.c
   A unified-stack abstract machine and garbage collector
   for list-C, a variant of micro-C with cons cells.
   sestoft@itu.dk * 2009-11-17, 2012-02-08

   Modified further to avoid mangling pointers by sparkes.
   A heavy handed approach is used.
   Previously, words were (implicitly) int32_ts.
   Now, words are structs containing data and a discriminant.
   For simplicity, data is an intptr_t.




   Compile like this, on ssh.itu.dk say:
      gcc -Wall listmachine.c -o listmachine

   If necessary, force compiler to use 32 bit integers:
      gcc -m32 -Wall listmachine.c -o listmachine

   To execute a program file using this abstract machine, do:
      ./listmachine <programfile> <arg1> <arg2> ...
   To get also a trace of the program execution:
      ./listmachine -trace <programfile> <arg1> <arg2> ...

   This code assumes -- and checks -- that values of type
   int, unsigned int and unsigned int* have size 32 bits.
*/

/*
   Data representation in the stack s[...] and the heap:
    * All integers are tagged with a 1 bit in the least significant
      position, regardless of whether they represent program integers,
      return addresses, array base addresses or old base pointers
      (into the stack).
    * All heap references are word-aligned, that is, the two least
      significant bits of a heap reference are 00.
    * Integer constants and code addresses in the program array
      p[...] are not tagged.
   The distinction between integers and references is necessary for
   the garbage collector to be precise (not conservative).

   The heap consists of 32-bit words, and the heap is divided into
   blocks.  A block has a one-word header block[0] followed by the
   block's contents: zero or more words block[i], i=1..n.

   A header has the form ttttttttnnnnnnnnnnnnnnnnnnnnnngg
   where tttttttt is the block tag, all 0 for cons cells
         nn....nn is the block length (excluding header)
         gg       is the block's color

   The block color has this meaning:
   gg=00=White: block is dead (after mark, before sweep)
   gg=01=Grey:  block is live, children not marked (during mark)
   gg=11=Black: block is live (after mark, before sweep)
   gg=11=Blue:  block is on the freelist or orphaned

   A block of length zero is an orphan; it cannot be used
   for data and cannot be on the freelist.  An orphan is
   created when allocating all but the last word of a free block.
*/

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>

// Heap + NULL -> HULL
#define HULL 0

const size_t HEAPSIZE = 10;   // Heap size in words
const size_t STACKSIZE = 1000; // Stack size

typedef enum {
  HDR = 0, // The header to a block
  INT = 1, // An integer
  PTR = 2, // A(n actual) pointer
} data_t;

// Set the word type as data and a discriminant.
typedef struct word {
  intptr_t data;
  data_t type;
} word_t;

// These numeric instruction codes must agree with ListC/Machine.fs:
// C23 enum with size intptr_t
typedef enum : intptr_t {
  CSTI = 0,
  ADD = 1,
  SUB = 2,
  MUL = 3,
  DIV = 4,
  MOD = 5,
  EQ = 6,
  LT = 7,
  NOT = 8,
  DUP = 9,
  SWAP = 10,
  LDI = 11,
  STI = 12,
  GETBP = 13,
  GETSP = 14,
  INCSP = 15,
  GOTO = 16,
  IFZERO = 17,
  IFNZRO = 18,
  CALL = 19,
  TCALL = 20,
  RET = 21,
  PRINTI = 22,
  PRINTC = 23,
  LDARGS = 24,
  STOP = 25,
  NIL = 26,
  CONS = 27,
  CAR = 28,
  CDR = 29,
  SETCAR = 30,
  SETCDR = 31,
} instr_t;

typedef enum Tag {
  TagCons = 0,
  TagFree = 1,
} tag_t;

typedef enum Color {
  White = 0,
  Grey = 1,
  Black = 2,
  Blue = 3
} color_t;

// As there's only a single translation unit (this source) we use static inline.

static inline void assert_word_type(data_t t, const word_t *w) {
  if (w->type != HDR) {
    printf("Expected %u, found %u with data %ld", t, w->type, w->data);
    exit(1);
  }
}

static inline int32_t BlockTag(const word_t *hdr_ptr) {
  assert_word_type(HDR, hdr_ptr);
  return hdr_ptr->data >> 24;
}

static inline size_t BlockLen(const word_t *hdr_ptr) {
  assert_word_type(HDR, hdr_ptr);
  return (((hdr_ptr->data) >> 2) & 0x003FFFFF);
}

static inline color_t BlockColor(const word_t *hdr_ptr) {
  assert_word_type(HDR, hdr_ptr);
  return ((hdr_ptr->data) & 3);
}

static inline void PaintBlock(word_t *hdr_ptr, color_t color) {
  assert_word_type(HDR, hdr_ptr);
  hdr_ptr->data = (((hdr_ptr->data) & (~3)) | (color));
  return;
}

word_t *heap;
word_t *afterHeap;
word_t *freelist;

// Print the stack machine instruction at p[pc]

void printInstruction(instr_t prg[], size_t prg_ctr) {
  switch (prg[prg_ctr]) {
  case CSTI:
    printf("CSTI %ld", prg[prg_ctr + 1]);
    break;
  case ADD:
    printf("ADD");
    break;
  case SUB:
    printf("SUB");
    break;
  case MUL:
    printf("MUL");
    break;
  case DIV:
    printf("DIV");
    break;
  case MOD:
    printf("MOD");
    break;
  case EQ:
    printf("EQ");
    break;
  case LT:
    printf("LT");
    break;
  case NOT:
    printf("NOT");
    break;
  case DUP:
    printf("DUP");
    break;
  case SWAP:
    printf("SWAP");
    break;
  case LDI:
    printf("LDI");
    break;
  case STI:
    printf("STI");
    break;
  case GETBP:
    printf("GETBP");
    break;
  case GETSP:
    printf("GETSP");
    break;
  case INCSP:
    printf("INCSP %ld", prg[prg_ctr + 1]);
    break;
  case GOTO:
    printf("GOTO %ld", prg[prg_ctr + 1]);
    break;
  case IFZERO:
    printf("IFZERO %ld", prg[prg_ctr + 1]);
    break;
  case IFNZRO:
    printf("IFNZRO %ld", prg[prg_ctr + 1]);
    break;
  case CALL:
    printf("CALL %ld %ld", prg[prg_ctr + 1], prg[prg_ctr + 2]);
    break;
  case TCALL:
    printf("TCALL %ld %ld %ld", prg[prg_ctr + 1], prg[prg_ctr + 2], prg[prg_ctr + 3]);
    break;
  case RET:
    printf("RET %ld", prg[prg_ctr + 1]);
    break;
  case PRINTI:
    printf("PRINTI");
    break;
  case PRINTC:
    printf("PRINTC");
    break;
  case LDARGS:
    printf("LDARGS");
    break;
  case STOP:
    printf("STOP");
    break;
  case NIL:
    printf("NIL");
    break;
  case CONS:
    printf("CONS");
    break;
  case CAR:
    printf("CAR");
    break;
  case CDR:
    printf("CDR");
    break;
  case SETCAR:
    printf("SETCAR");
    break;
  case SETCDR:
    printf("SETCDR");
    break;
  default:
    printf("<unknown>");
    break;
  }
}

void printHeader(word_t *hdr) {
  size_t length = BlockLen(hdr);
  color_t color = BlockColor(hdr);
  tag_t tag = BlockTag(hdr);
  printf("H #{%ld} [%d %zu %d]\n", (intptr_t)hdr, tag, length, color);
}

void printHeap() {
  word_t *idx = heap;

  printf("freelist: #{%ld}\n", (intptr_t)freelist);

  while (idx < afterHeap) {

    size_t length = BlockLen(idx);
    color_t color = BlockColor(idx);
    printHeader(idx);

    ++idx; // Discard the header

    if (color == White) {
      for (int i = 0; i < length; ++i) {
        printf(" [%d] ", i);
        switch (idx[i].type) {
        case HDR: {
          printHeader(idx + i);
        } break;
        case INT: {
          printf("%ld\n", idx[i].data);
        } break;
        case PTR: {
          printf("#{%ld}\n", idx[i].data);
        } break;
        }
      }
    }

    idx += length;
  }
  printf("\n");
}

void printStack(word_t stk[], int stk_ptr) {
  printf("[ ");
  for (int i = 0; i <= stk_ptr; i++) {
    switch (stk[i].type) {

    case HDR:
      printf("Header on stack");
      exit(1);
    case INT: {
      printf("%ld ", stk[i].data);
    } break;
    case PTR: {
      printf("#{%ld} ", stk[i].data);
    } break;
    }
  }
  printf("]");
}

// Print current stack (marking heap references by #) and current instruction
void printStackAndPc(word_t stk[], int base_ptr, int stk_ptr, instr_t prg[], size_t prg_ctr) {
  printStack(stk, stk_ptr);
  printf(" {%zu:", prg_ctr);
  printInstruction(prg, prg_ctr);
  printf("}\n");
}

// Read instructions from a file, return array of instructions
instr_t *readfile(char *filename) {
  size_t capacity = 1;
  size_t size = 0;

  instr_t *program = malloc(sizeof(word_t) * capacity);

  FILE *inp = fopen(filename, "r");

  instr_t instr;

  while (fscanf(inp, "%ld", &instr) == 1) {
    if (size >= capacity) {
      instr_t *buffer = malloc(sizeof(word_t) * 2 * capacity);

      for (int i = 0; i < capacity; i++) {
        buffer[i] = program[i];
      }

      free(program);
      program = buffer;
      capacity *= 2;
    }

    program[size++] = instr;
  }

  fclose(inp);
  return program;
}

word_t *allocate(tag_t tag, size_t length, word_t stk[], int stk_ptr, bool trace);

// The machine: execute the code starting at p[pc]

int execcode(instr_t prg[], word_t stk[], int iargs[], int iargc, bool trace) {
  int bse_ptr = -999; // Base pointer, for local variable access
  int stk_ptr = -1;   // Stack top pointer
  size_t prg_ctr = 0; // Program counter: next instruction
  for (;;) {
    if (trace) {
      printStackAndPc(stk, bse_ptr, stk_ptr, prg, prg_ctr);
    }

    switch (prg[prg_ctr++]) {
    case CSTI: {
      word_t word = {.data = prg[prg_ctr++], .type = INT};
      stk[stk_ptr + 1] = word;
      stk_ptr++;
    } break;
    case ADD: {
      word_t word = {.data = stk[stk_ptr - 1].data + stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case SUB: {
      word_t word = {.data = stk[stk_ptr - 1].data - stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case MUL: {
      word_t word = {.data = stk[stk_ptr - 1].data * stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case DIV: {
      word_t word = {.data = stk[stk_ptr - 1].data / stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case MOD: {
      word_t word = {.data = stk[stk_ptr - 1].data % stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case EQ: {
      word_t word = {.data = stk[stk_ptr - 1].data == stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case LT: {
      word_t word = {.data = stk[stk_ptr - 1].data < stk[stk_ptr].data, .type = INT};
      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case NOT: {
      int v = stk[stk_ptr].data;
      word_t word = {.data = (v == 0 ? 1 : 0), .type = INT};
      stk[stk_ptr] = word;
    } break;
    case DUP: {
      stk[stk_ptr + 1] = stk[stk_ptr];
      stk_ptr++;
    } break;
    case SWAP: {
      word_t tmp = stk[stk_ptr];
      stk[stk_ptr] = stk[stk_ptr - 1];
      stk[stk_ptr - 1] = tmp;
    } break;
    case LDI: { // load indirect
      stk[stk_ptr] = stk[stk[stk_ptr].data];
    } break;
    case STI: { // store indirect, keep value on top
      stk[stk[stk_ptr - 1].data] = stk[stk_ptr];
      stk[stk_ptr - 1] = stk[stk_ptr];
      stk_ptr--;
    } break;
    case GETBP: {
      word_t word = {.data = bse_ptr, .type = INT};
      stk[stk_ptr + 1] = word;
      stk_ptr++;
    } break;
    case GETSP: {
      word_t word = {.data = stk_ptr, .type = INT};
      stk[stk_ptr + 1] = word;
      stk_ptr++;
    } break;
    case INCSP: {
      stk_ptr = stk_ptr + prg[prg_ctr++];
    } break;
    case GOTO: {
      prg_ctr = prg[prg_ctr];
    } break;
    case IFZERO: {
      int v = stk[stk_ptr--].data;
      prg_ctr = (v == 0) ? prg[prg_ctr] : prg_ctr + 1;
    } break;
    case IFNZRO: {
      int v = stk[stk_ptr--].data;
      prg_ctr = (v != 0) ? prg[prg_ctr] : prg_ctr + 1;
    } break;
    case CALL: {
      int argc = prg[prg_ctr++];
      for (int i = 0; i < argc; i++) {           // Make room for return address
        stk[stk_ptr - i + 2] = stk[stk_ptr - i]; // and old base pointer
      }
      word_t word_a = {.data = prg_ctr + 1, .type = INT};
      stk[stk_ptr - argc + 1] = word_a;
      stk_ptr++;

      word_t word_b = {.data = bse_ptr, .type = INT};
      stk[stk_ptr - argc + 1] = word_b;
      stk_ptr++;
      bse_ptr = stk_ptr + 1 - argc;
      prg_ctr = prg[prg_ctr];

    } break;
    case TCALL: {
      int argc = prg[prg_ctr++];            // Number of new arguments
      int pop = prg[prg_ctr++];             // Number of variables to discard
      for (int i = argc - 1; i >= 0; i--) { // Discard variables
        stk[stk_ptr - i - pop] = stk[stk_ptr - i];
      }
      stk_ptr = stk_ptr - pop;
      prg_ctr = prg[prg_ctr];
    } break;
    case RET: {
      word_t res = stk[stk_ptr];

      stk_ptr = stk_ptr - prg[prg_ctr];
      bse_ptr = stk[--stk_ptr].data;
      prg_ctr = stk[--stk_ptr].data;

      stk[stk_ptr] = res;
    } break;
    case PRINTI:

      switch (stk[stk_ptr].type) {

      case HDR:
        printf("Header on the stack");
        exit(1);
      case INT: {
        printf("%ld ", stk[stk_ptr].data);
      } break;
      case PTR: {
        printf("#{%ld} ", stk[stk_ptr].data);
      } break;
      }

      break;
    case PRINTC:
      switch (stk[stk_ptr].type) {

      case HDR:
        printf("Header on the stack");
        exit(1);
      case INT: {
        printf("%ld ", stk[stk_ptr].data);
      } break;
      case PTR: {
        printf("#{%ld} ", stk[stk_ptr].data);
      } break;
      }

      break;
    case LDARGS: {
      for (int i = 0; i < iargc; i++) { // Push commandline arguments
        word_t word = {.data = iargs[i], .type = INT};
        stk[++stk_ptr] = word;
      }
    } break;
    case STOP:
      return 0;
    case NIL: {
      word_t word = {.data = 0, .type = INT};
      stk[stk_ptr + 1] = word;
      stk_ptr++;
    } break;
    case CONS: {
      word_t *ptr = allocate(TagCons, 2, stk, stk_ptr, trace);
      ptr[1] = (word_t)stk[stk_ptr - 1];
      ptr[2] = (word_t)stk[stk_ptr];

      word_t word = {.data = (intptr_t)ptr, .type = PTR};

      stk[stk_ptr - 1] = word;
      stk_ptr--;
    } break;
    case CAR: {
      assert(stk[stk_ptr].type == PTR);
      word_t *data = (word_t *)stk[stk_ptr].data;

      if (data == HULL) {
        printf("Cannot take car of null\n");
        return -1;
      }

      stk[stk_ptr] = data[1];
    } break;
    case CDR: {
      assert(stk[stk_ptr].type == PTR);
      word_t *data = (word_t *)stk[stk_ptr].data;

      if (data == HULL) {
        printf("Cannot take cdr of null\n");
        return -1;
      }

      stk[stk_ptr] = data[2];
    } break;
    case SETCAR: {
      word_t val = stk[stk_ptr--];
      assert(stk[stk_ptr].type == PTR);

      word_t *ptr = (word_t *)stk[stk_ptr].data;
      ptr[1] = val;
    } break;
    case SETCDR: {
      word_t val = (word_t)stk[stk_ptr--];
      assert(stk[stk_ptr].type == PTR);

      word_t *ptr = (word_t *)stk[stk_ptr].data;
      ptr[2] = val;
    } break;
    default:
      printf("Illegal instruction %ld at address %zu\n", prg[prg_ctr - 1],
             prg_ctr - 1);
      return -1;
    }
  }
}

// Read program from file, and execute it
int execute(int argc, char **argv, bool trace) {
  instr_t *prg = readfile(argv[trace ? 2 : 1]);     // program bytecodes: int[]
  word_t *stk = malloc(sizeof(word_t) * STACKSIZE); // stack: int[]

  int iargc = trace ? argc - 3 : argc - 2;
  int *iargs = malloc(sizeof(int) * iargc); // program inputs: int[]

  for (int i = 0; i < iargc; i++) { // Convert commandline arguments
    iargs[i] = atoi(argv[trace ? i + 3 : i + 2]);
  }
  // Measure cpu time for executing the program
  struct rusage ru1;
  struct rusage ru2;

  getrusage(RUSAGE_SELF, &ru1);
  int res = execcode(prg, stk, iargs, iargc, trace); // Execute program proper
  getrusage(RUSAGE_SELF, &ru2);
  struct timeval t1 = ru1.ru_utime, t2 = ru2.ru_utime;
  double runtime =
      t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) / 1000000.0;
  /* printf("\nUsed %7.3f cpu seconds\n", runtime); */
  return res;
}

// Garbage collection and heap allocation

word_t mkheader(tag_t tag, size_t length, color_t color) {
  intptr_t data = (tag << 24) | (length << 2) | color;
  word_t word = {.data = data, .type = HDR};

  return word;
}

int inHeap(word_t *p) { return heap <= p && p < afterHeap; }

// Call this after a GC to get heap statistics:
void heapStatistics() {
  int blocks = 0;
  int free = 0;
  int orphans = 0;
  int blocksSize = 0;
  int freeSize = 0;
  int largestFree = 0;

  word_t *heapPtr = heap;

  while (heapPtr < afterHeap) {
    if (BlockLen(heapPtr) > 0) {
      blocks++;
      blocksSize += BlockLen(heapPtr);
    } else {
      orphans++;
    }

    word_t *nextBlock = heapPtr + BlockLen(heapPtr) + 1;
    if (nextBlock > afterHeap) {
      printf("HEAP ERROR: block at heap[%ld] extends beyond heap\n", heapPtr - heap);
      exit(-1);
    }

    heapPtr = nextBlock;
  }

  word_t *freePtr = freelist;
  while (freePtr != HULL) {
    free++;
    int length = BlockLen(freePtr);
    if (freePtr < heap || afterHeap < freePtr + length + 1) {
      printf("HEAP ERROR: freelist item %d (at heap[%ld], length %d) is outside heap\n",
             free, freePtr - heap, length);
      exit(-1);
    }
    freeSize += length;
    largestFree = length > largestFree ? length : largestFree;
    if (BlockColor(freePtr) != Blue) {
      printf("Non-blue block at heap[%ld] on freelist\n", (intptr_t)freePtr);
    }

    assert(freePtr[1].type == PTR);
    freePtr = (word_t *)freePtr[1].data;
  }

  printf("Heap: %d blocks (%d words); of which %d free (%d words, largest %d words); %d orphans\n",
         blocks, blocksSize, free, freeSize, largestFree, orphans);
  printHeap();
}

void initheap() {
  heap = (word_t *)malloc(sizeof(word_t) * HEAPSIZE);
  afterHeap = &heap[HEAPSIZE];
  // Initially, entire heap is one block on the freelist:
  heap[0] = mkheader(TagFree, HEAPSIZE - 1, Blue);

  freelist = &heap[0]; // the contents of freelist is a pointer to the start of the heap

  word_t word = {.data = HULL, .type = PTR};
  *(freelist + 1) = word; // the next block in the freelist chain is initially set to `null`
}

// mark recurisve, recursive case
void markRecursiveR(word_t *blk_ptr) {

  color_t color = BlockColor(blk_ptr);

  if (color == White) {
    PaintBlock(blk_ptr, Black);

    for (int i = 1; i <= BlockLen(blk_ptr); ++i) {
      if (blk_ptr[i].type == PTR && blk_ptr[i].data != HULL) {
        markRecursiveR((word_t *)blk_ptr[i].data);
      }
    }
  }
}

// mark recurisve, base case
void markRecursiveB(word_t stk[], int stk_ptr, bool trace) {
  for (int i = stk_ptr; 0 <= i; --i) {
    if (stk[i].type == PTR) {
      markRecursiveR((word_t *)stk[i].data);
    }
  }
}

void markPhase(word_t stk[], int stk_ptr, bool trace) {
  if (trace) {
    printf("marking ...\n");
  }

  markRecursiveB(stk, stk_ptr, trace);

  if (trace) {
    printf("marking complete\n");
  }
}

void sweepPhase(bool trace) {
  if (trace) {
    printf("sweeping ...\n");
  }

  word_t *hdr = heap;
  while (hdr < afterHeap) {
    switch (BlockColor(hdr)) {

    case Blue:    // Same as white
    case White: { // Add the block to the start of the freelist
      word_t fl_nxt = {.data = (intptr_t)freelist, .type = PTR};
      freelist = hdr;           // Update the freelist to start at the block.
      *(freelist + 1) = fl_nxt; // Set next freelist pointer after the header to the block.

      // Merge the following blocks, until HULL or non-free.
      word_t *next = freelist + 1 + BlockLen(freelist);

      while (next < afterHeap && next->data != HULL) {
        assert_word_type(HDR, next);

        color_t next_color = BlockColor(next);
        if (next_color == White || next_color == Blue) {
          size_t offset = BlockLen(freelist) + 1 + BlockLen(next);
          word_t fresh_hdr = mkheader(TagFree, offset, White);
          *freelist = fresh_hdr;

          next = next + BlockLen(next) + 1;
        } else {
          break;
        }
      }

    } break;
    case Grey:
      printf("Grey block found");
      exit(1);
    case Black:
      PaintBlock(hdr, White);
      break;
    }

    size_t len = BlockLen(hdr);
    hdr += (1 + len);
  }

  if (trace) {
    printf("sweep complete\n");
    printf("compacting...\n");
  }

  if (freelist == HULL) {
    printf("Freelist HULL\n");
    return;
  }

  if (trace) {
    printHeader(freelist);
  }
}

void collect(word_t stk[], int stk_ptr, bool trace) {
  markPhase(stk, stk_ptr, trace);
  if (trace) {
    heapStatistics();
  }
  sweepPhase(trace);
  if (trace) {
    heapStatistics();
  }
}

// Setup:
// - freeblock points to the next free block.
// - freeblock + 1 contains a pointer to the following block (or null / 0).
// Action:
// - Start at the first free block, and then continue working through freeblocks
// by the stored pointer until a block with sufficient length is found.
// - When a block with sufficient length is found, update the freelist pointer,
// - Maybe split the block and update pointers.
// - Return a pointer to the block found.
word_t *allocate(tag_t tag, size_t length, word_t stk[], int stk_ptr, bool trace) {
  size_t attempt = 1;

  do {
    word_t *free = freelist;

    while (free != HULL) {

      int remaining = BlockLen(free) - length;

      if (remaining >= 0) {
        // if exhaust block, use stored pointer to next block
        // otherwise, create new block and copy stored pointer over

        if (remaining == 0) { // Exact fit with free block, so use stored pointer.
          freelist = (word_t *)(*(free + 1)).data;
        } else if (remaining == 1) {               // Fill with unusable block of legnth zero
          freelist = (word_t *)(*(free + 1)).data; // So, use stored pointer for next next block.
          // Create unusable block through header with zero length.
          *(free + length + 1) = mkheader(TagFree, 0, Blue);

        } else {                          // Block will not be filled as excess length.
          freelist = (free + length + 1); // Set freelist to after length used.
          // Set a new header with the remaining capacity.
          *(free + length + 1) = mkheader(TagFree, remaining - 1, Blue);
          *(free + length + 2) = *(free + 1); // Copy over the stored pointer.
        }

        *free = mkheader(tag, length, White);
        return free;
      }

      free = (word_t *)(*(free + 1)).data; // No capacity so use stored pointer to next block.
    }

    // On first attempt, if no free space, do a garbage collection and retry
    if (attempt == 1) {
      collect(stk, stk_ptr, trace);
    }

  } while (attempt++ == 1);

  printf("Out of memory\n");
  exit(1);
}

// Read code from file and execute it

int main(int argc, char **argv) {

  if (sizeof(intptr_t) < 4) {

    printf("Size of intptr_t lower than 32 bit, cannot run\n");
    return -1;

  } else if (argc < 2) {

    printf("Usage: listmachine [--trace] <programfile> <arg1> ...\n");
    return -1;

  } else {

    bool trace = argc >= 3 && 0 == strncmp(argv[1], "--trace", 7);
    initheap();

    if (trace) {
      printHeap();
    }

    int result = execute(argc, argv, trace);

    return result;
  }
}
