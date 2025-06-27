/* File ListC/listmachine.c
   A unified-stack abstract machine and garbage collector
   for list-C, a variant of micro-C with cons cells.
   sestoft@itu.dk * 2009-11-17, 2012-02-08

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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>

const size_t HEAPSIZE = 1000;  // Heap size in words
const size_t STACKSIZE = 1000; // Stack size

// Set the word type equal in size to a pointer.
typedef intptr_t word;

// These numeric instruction codes must agree with ListC/Machine.fs:
// C23 enum with size intptr_t
typedef enum : intptr_t { CSTI = 0,
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

typedef enum Tag { TagCons = 0,
} tag_t;

typedef enum Color { White = 0,
                     Grey = 1,
                     Black = 2,
                     Blue = 3
} color_t;

// As there's only a single translation unit (this source) we use static inline.
static inline bool IsInstr(instr_t inst) {
  return (inst & 1) == 1;
}

static inline instr_t TagInt(int instr) {
  return (((instr_t)instr) << 1) | 1;
}

static inline int UntagInstr(instr_t inst) { return inst >> 1; }

static inline word BlockTag(word hdr) { return hdr >> 24; }

static inline word Length(word hdr) {
  return (((hdr) >> 2) & 0x003FFFFF);
}

static inline color_t GetColor(word hdr) {
  return ((hdr) & 3);
}

static inline word Paint(word hdr, color_t color) {
  return (((hdr) & (~3)) | (color));
}

word *heap;
word *afterHeap;
word *freelist;

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

// Print current stack (marking heap references by #) and current instruction
void printStackAndPc(instr_t stk[], int base_ptr, int stk_ptr, instr_t prg[], size_t prg_ctr) {

  printf("[ ");
  for (int i = 0; i <= stk_ptr; i++) {
    (IsInstr(stk[i])) ? printf("%d ", UntagInstr(stk[i])) : printf("#%ld ", stk[i]);
  }
  printf("] {%zu:", prg_ctr);
  printInstruction(prg, prg_ctr);
  printf("}\n");
}

// Read instructions from a file, return array of instructions
instr_t *readfile(char *filename) {
  size_t capacity = 1;
  size_t size = 0;

  instr_t *program = malloc(sizeof(instr_t) * capacity);

  FILE *inp = fopen(filename, "r");

  instr_t instr;

  while (fscanf(inp, "%ld", &instr) == 1) {
    if (size >= capacity) {
      instr_t *buffer = malloc(sizeof(instr_t) * 2 * capacity);

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

word *allocate(uint32_t tag, size_t length, instr_t stk[], int stk_ptr);

// The machine: execute the code starting at p[pc]

int execcode(instr_t prg[], instr_t stk[], int iargs[], int iargc, bool trace) {
  int bse_ptr = -999; // Base pointer, for local variable access
  int stk_ptr = -1;   // Stack top pointer
  size_t prg_ctr = 0; // Program counter: next instruction
  for (;;) {
    if (trace) {
      printStackAndPc(stk, bse_ptr, stk_ptr, prg, prg_ctr);
    }

    switch (prg[prg_ctr++]) {
    case CSTI:
      stk[stk_ptr + 1] = TagInt(prg[prg_ctr++]);
      stk_ptr++;
      break;
    case ADD:
      stk[stk_ptr - 1] = TagInt(UntagInstr(stk[stk_ptr - 1]) + UntagInstr(stk[stk_ptr]));
      stk_ptr--;
      break;
    case SUB:
      stk[stk_ptr - 1] = TagInt(UntagInstr(stk[stk_ptr - 1]) - UntagInstr(stk[stk_ptr]));
      stk_ptr--;
      break;
    case MUL:
      stk[stk_ptr - 1] = TagInt(UntagInstr(stk[stk_ptr - 1]) * UntagInstr(stk[stk_ptr]));
      stk_ptr--;
      break;
    case DIV:
      stk[stk_ptr - 1] = TagInt(UntagInstr(stk[stk_ptr - 1]) / UntagInstr(stk[stk_ptr]));
      stk_ptr--;
      break;
    case MOD:
      stk[stk_ptr - 1] = TagInt(UntagInstr(stk[stk_ptr - 1]) % UntagInstr(stk[stk_ptr]));
      stk_ptr--;
      break;
    case EQ:
      stk[stk_ptr - 1] = TagInt(stk[stk_ptr - 1] == stk[stk_ptr] ? 1 : 0);
      stk_ptr--;
      break;
    case LT:
      stk[stk_ptr - 1] = TagInt(stk[stk_ptr - 1] < stk[stk_ptr] ? 1 : 0);
      stk_ptr--;
      break;
    case NOT: {
      int v = stk[stk_ptr];
      stk[stk_ptr] = TagInt((IsInstr(v) ? UntagInstr(v) == 0 : v == 0) ? 1 : 0);
    } break;
    case DUP:
      stk[stk_ptr + 1] = stk[stk_ptr];
      stk_ptr++;
      break;
    case SWAP: {
      int tmp = stk[stk_ptr];
      stk[stk_ptr] = stk[stk_ptr - 1];
      stk[stk_ptr - 1] = tmp;
    } break;
    case LDI: // load indirect
      stk[stk_ptr] = stk[UntagInstr(stk[stk_ptr])];
      break;
    case STI: // store indirect, keep value on top
      stk[UntagInstr(stk[stk_ptr - 1])] = stk[stk_ptr];
      stk[stk_ptr - 1] = stk[stk_ptr];
      stk_ptr--;
      break;
    case GETBP:
      stk[stk_ptr + 1] = TagInt(bse_ptr);
      stk_ptr++;
      break;
    case GETSP:
      stk[stk_ptr + 1] = TagInt(stk_ptr);
      stk_ptr++;
      break;
    case INCSP:
      stk_ptr = stk_ptr + prg[prg_ctr++];
      break;
    case GOTO:
      prg_ctr = prg[prg_ctr];
      break;
    case IFZERO: {
      int v = stk[stk_ptr--];
      prg_ctr =
          (IsInstr(v) ? UntagInstr(v) == 0 : v == 0) ? prg[prg_ctr] : prg_ctr + 1;
    } break;
    case IFNZRO: {
      int v = stk[stk_ptr--];
      prg_ctr =
          (IsInstr(v) ? UntagInstr(v) != 0 : v != 0) ? prg[prg_ctr] : prg_ctr + 1;
    } break;
    case CALL: {
      int argc = prg[prg_ctr++];
      for (int i = 0; i < argc; i++) {           // Make room for return address
        stk[stk_ptr - i + 2] = stk[stk_ptr - i]; // and old base pointer
      }
      stk[stk_ptr - argc + 1] = TagInt(prg_ctr + 1);
      stk_ptr++;
      stk[stk_ptr - argc + 1] = TagInt(bse_ptr);
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
      int res = stk[stk_ptr];
      stk_ptr = stk_ptr - prg[prg_ctr];
      bse_ptr = UntagInstr(stk[--stk_ptr]);
      prg_ctr = UntagInstr(stk[--stk_ptr]);
      stk[stk_ptr] = res;
    } break;
    case PRINTI:
      printf("%ld ", IsInstr(stk[stk_ptr]) ? UntagInstr(stk[stk_ptr]) : stk[stk_ptr]);
      break;
    case PRINTC:
      printf("%d", UntagInstr(stk[stk_ptr]));
      break;
    case LDARGS: {
      for (int i = 0; i < iargc; i++) { // Push commandline arguments
        stk[++stk_ptr] = TagInt(iargs[i]);
      }
    } break;
    case STOP:
      return 0;
    case NIL:
      stk[stk_ptr + 1] = 0;
      stk_ptr++;
      break;
    case CONS: {
      word *p = allocate(TagCons, 2, stk, stk_ptr);
      p[1] = (word)stk[stk_ptr - 1];
      p[2] = (word)stk[stk_ptr];
      stk[stk_ptr - 1] = (instr_t)p;
      stk_ptr--;
    } break;
    case CAR: {
      word *p = (word *)stk[stk_ptr];
      if (p == 0) {
        printf("Cannot take car of null\n");
        return -1;
      }
      stk[stk_ptr] = (int)(p[1]);
    } break;
    case CDR: {
      word *p = (word *)stk[stk_ptr];
      if (p == 0) {
        printf("Cannot take cdr of null\n");
        return -1;
      }
      stk[stk_ptr] = (int)(p[2]);
    } break;
    case SETCAR: {
      word v = (word)stk[stk_ptr--];
      word *p = (word *)stk[stk_ptr];
      p[1] = v;
    } break;
    case SETCDR: {
      word v = (word)stk[stk_ptr--];
      word *p = (word *)stk[stk_ptr];
      p[2] = v;
    } break;
    default:
      printf("Illegal instruction %ld at address %zu\n", prg[prg_ctr - 1], prg_ctr - 1);
      return -1;
    }
  }
}

// Read program from file, and execute it
int execute(int argc, char **argv, bool trace) {
  instr_t *prg = readfile(argv[trace ? 2 : 1]);       // program bytecodes: int[]
  instr_t *stk = malloc(sizeof(instr_t) * STACKSIZE); // stack: int[]

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
  printf("\nUsed %7.3f cpu seconds\n", runtime);
  return res;
}

// Garbage collection and heap allocation

word mkheader(unsigned int tag, unsigned int length, unsigned int color) {
  return (tag << 24) | (length << 2) | color;
}

int inHeap(word *p) { return heap <= p && p < afterHeap; }

// Call this after a GC to get heap statistics:
void heapStatistics() {
  int blocks = 0;
  int free = 0;
  int orphans = 0;
  int blocksSize = 0;
  int freeSize = 0;
  int largestFree = 0;

  word *heapPtr = heap;

  while (heapPtr < afterHeap) {
    if (Length(heapPtr[0]) > 0) {
      blocks++;
      blocksSize += Length(heapPtr[0]);
    } else {
      orphans++;
    }

    word *nextBlock = heapPtr + Length(heapPtr[0]) + 1;
    if (nextBlock > afterHeap) {
      printf("HEAP ERROR: block at heap[%ld] extends beyond heap\n", heapPtr - heap);
      exit(-1);
    }

    heapPtr = nextBlock;
  }

  word *freePtr = freelist;
  while (freePtr != 0) {
    free++;
    int length = Length(freePtr[0]);
    if (freePtr < heap || afterHeap < freePtr + length + 1) {
      printf("HEAP ERROR: freelist item %d (at heap[%ld], length %d) is outside heap\n", free, freePtr - heap, length);
      exit(-1);
    }
    freeSize += length;
    largestFree = length > largestFree ? length : largestFree;
    if (GetColor(freePtr[0]) != Blue) {
      printf("Non-blue block at heap[%ld] on freelist\n", freePtr);
    }
    freePtr = (word *)freePtr[1];
  }

  printf("Heap: %d blocks (%d words); of which %d free (%d words, largest %d words); %d orphans\n", blocks, blocksSize, free, freeSize, largestFree, orphans);
}

void initheap() {
  heap = (word *)malloc(sizeof(word) * HEAPSIZE);
  afterHeap = &heap[HEAPSIZE];
  // Initially, entire heap is one block on the freelist:
  heap[0] = mkheader(0, HEAPSIZE - 1, Blue);
  heap[1] = (word)0;
  freelist = &heap[0];
}

void markPhase(instr_t stk[], int stk_ptr) {
  printf("marking ...\n");
  // TODO: Actually mark something
}

void sweepPhase() {
  printf("sweeping ...\n");
  // TODO: Actually sweep
}

void collect(instr_t stk[], int stk_ptr) {
  markPhase(stk, stk_ptr);
  heapStatistics();
  sweepPhase();
  heapStatistics();
}

word *allocate(uint32_t tag, size_t length, instr_t stk[], int stk_ptr) {
  size_t attempt = 1;

  do {
    word *free = freelist;
    word **prev = &freelist;

    while (free != 0) {
      int rest = Length(free[0]) - length;
      if (rest >= 0) {
        if (rest == 0) { // Exact fit with free block
          *prev = (word *)free[1];
        } else if (rest == 1) { // Create orphan (unusable) block
          *prev = (word *)free[1];
          free[length + 1] = mkheader(0, rest - 1, Blue);
        } else { // Make previous free block point to rest of this block
          *prev = &free[length + 1];
          free[length + 1] = mkheader(0, rest - 1, Blue);
          free[length + 2] = free[1];
        }
        free[0] = mkheader(tag, length, White);
        return free;
      }
      prev = (word **)&free[1];
      free = (word *)free[1];
    }

    // No free space, do a garbage collection and try again
    if (attempt == 1) {
      collect(stk, stk_ptr);
    }
  } while (attempt++ == 1);

  printf("Out of memory\n");
  exit(1);
}

// Read code from file and execute it

int main(int argc, char **argv) {

  if (sizeof(intptr_t) < 4) {

    printf("Size of word, word* or int lower than 32 bit, cannot run\n");
    return -1;

  } else if (argc < 2) {

    printf("Usage: listmachine [--trace] <programfile> <arg1> ...\n");
    return -1;

  } else {

    bool trace = argc >= 3 && 0 == strncmp(argv[1], "--trace", 7);
    initheap();
    return execute(argc, argv, trace);
  }
}
