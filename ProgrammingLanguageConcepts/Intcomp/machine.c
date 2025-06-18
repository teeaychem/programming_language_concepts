#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum Code {
  SCST = 0,
  SVAR = 1,
  SADD = 2,
  SSUB = 3,
  SMUL = 4,
  SPOP = 5,
  SSWAP = 6
};

struct Machine {
  int capacity;
  int *stack;
  int stack_ptr;
  int curr_instr;
};

int seval(int code[], int size, struct Machine *m) {
  int prg_counter = 0;

  while (prg_counter < size) {
    switch (code[prg_counter++]) {
    case SCST: {
      assert(m->stack_ptr + 1 < m->capacity);

      m->stack[m->stack_ptr + 1] = code[prg_counter++];
      m->stack_ptr++;
    } break;

    case SVAR: {
      assert(m->stack_ptr + 1 < m->capacity);

      m->stack[m->stack_ptr + 1] = m->stack[m->stack_ptr - code[prg_counter++]];
      m->stack_ptr++;
    } break;

    case SADD: {
      assert(0 < m->stack_ptr);

      m->stack[m->stack_ptr - 1] =
          m->stack[m->stack_ptr - 1] + m->stack[m->stack_ptr];
      m->stack_ptr--;
    } break;

    case SSUB: {
      assert(0 < m->stack_ptr);

      m->stack[m->stack_ptr - 1] =
          m->stack[m->stack_ptr - 1] - m->stack[m->stack_ptr];
      m->stack_ptr--;
    } break;

    case SMUL: {
      assert(0 < m->stack_ptr);

      m->stack[m->stack_ptr - 1] =
          m->stack[m->stack_ptr - 1] * m->stack[m->stack_ptr];
      m->stack_ptr--;
    } break;

    case SPOP: {
      m->stack_ptr--;
    } break;

    case SSWAP: {
      assert(0 < m->stack_ptr);

      int tmp = m->stack[m->stack_ptr];
      m->stack[m->stack_ptr] = m->stack[m->stack_ptr - 1];
      m->stack[m->stack_ptr - 1] = tmp;

    } break;

    default:
      printf("Illegal instruction: %d at address %d", code[prg_counter - 1],
             prg_counter - 1);
      exit(-1);
    }
  }

  return m->stack[m->stack_ptr];
}

struct Machine default_machine() {
  int capacity = 100;
  struct Machine m = {.capacity = capacity, .stack = malloc(capacity * sizeof(int)), .stack_ptr = -1};

  return m;
}

void test_rpn1() {
  int rpn1[] = {SCST, 17, SVAR, 0, SVAR, 1, SADD, SSWAP, SPOP};

  struct Machine rpn1m = default_machine();
  int result = seval(rpn1, 9, &rpn1m);

  printf("rpn1: %d\n", result);
  assert(result == 34);
}

void test_rpn2() {
  int rpn2[] = {SCST, 17, SCST, 22, SCST, 100, SVAR, 1,
                SMUL, SSWAP, SPOP, SVAR, 1, SADD, SSWAP, SPOP};

  struct Machine rpn1m = default_machine();
  int result = seval(rpn2, 16, &rpn1m);

  printf("rpn2: %d\n", result);
  assert(result == 2217);
}

int main(int argc, char *argv[]) {

  struct Machine m = default_machine();

  if (argc == 1) {
    printf("Run with '-t' for tests or the path to a file.");
  }

  if (strcmp(argv[1], "-t") == 0) {
    test_rpn1();
    test_rpn2();
    exit(0);
  }

  int inst;
  int inst_count = 0;
  int max_insts = 1000;
  int *insts = malloc(max_insts);

  char *file_name = argv[1];
  FILE *file = fopen(file_name, "r");

  if (file == NULL) {
    printf("Unable to open file: %s\n", file_name);
    exit(-2);
  }

  while (!feof(file)) {
    int scan_code = fscanf(file, "%d", &inst);
    if (scan_code == 1) {
      if (inst_count == max_insts) {
        printf("Too many instructions (max %d)", max_insts);
        exit(-3);
      }

      insts[inst_count] = inst;
      inst_count++;
    }
  }

  for (int i = 0; i < inst_count; ++i) {
    printf("%d ", insts[i]);
  }
  printf("\n");

  struct Machine rpn1m = default_machine();
  int result = seval(insts, inst_count, &rpn1m);

  printf("Result: %d\n", result);
}
