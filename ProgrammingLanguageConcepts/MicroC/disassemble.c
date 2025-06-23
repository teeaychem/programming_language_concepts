#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int *array;
  size_t size;
  size_t capacity;
  size_t pc;

} IntVec;

void init_IntVec(IntVec *v, size_t capacity) {
  v->array = malloc(capacity * sizeof(int));
  v->size = 0;
  v->capacity = capacity;
}

void push_IntVec(IntVec *v, int elem) {
  if (v->size == v->capacity) {
    v->capacity *= 2;
    v->array = realloc(v->array, v->capacity * sizeof(int));
  }
  v->array[v->size] = elem;
  v->size++;
}

void free_IntVec(IntVec *v) {
  free(v->array);
  v->array = NULL;
  v->size = 0;
}

void print_IntVec(IntVec *v) {
  printf("[ ");
  for (int i = 0; i < v->size; ++i) {
    printf("%d ", v->array[i]);
  }
  printf("]");
  printf("\n");
}

#define CSTI 0
#define ADD 1
#define SUB 2
#define MUL 3
#define DIV 4
#define MOD 5
#define EQ 6
#define LT 7
#define NOT 8
#define DUP 9
#define SWAP 10
#define LDI 11
#define STI 12
#define GETBP 13
#define GETSP 14
#define INCSP 15
#define GOTO 16
#define IFZERO 17
#define IFNZRO 18
#define CALL 19
#define TCALL 20
#define RET 21
#define PRINTI 22
#define PRINTC 23
#define LDARGS 24
#define STOP 25

void readInst(int p[], int *pc, char buf[]) {
  int inst = p[*pc];
  (*pc)++;
  switch (inst) {
  case CSTI:
    sprintf(buf, "CSTI %d", p[(*pc)++]);
    break;
  case ADD:
    sprintf(buf, "ADD");
    break;
  case SUB:
    sprintf(buf, "SUB");
    break;
  case MUL:
    sprintf(buf, "MUL");
    break;
  case DIV:
    sprintf(buf, "DIV");
    break;
  case MOD:
    sprintf(buf, "MOD");
    break;
  case EQ:
    sprintf(buf, "EQ");
    break;
  case LT:
    sprintf(buf, "LT");
    break;
  case NOT:
    sprintf(buf, "NOT");
    break;
  case DUP:
    sprintf(buf, "DUP");
    break;
  case SWAP:
    sprintf(buf, "SWAP");
    break;
  case LDI:
    sprintf(buf, "LDI");
    break;
  case STI:
    sprintf(buf, "STI");
    break;
  case GETBP:
    sprintf(buf, "GETBP");
    break;
  case GETSP:
    sprintf(buf, "GETSP");
    break;
  case INCSP:
    sprintf(buf, "INCSP %d", p[(*pc)++]);
    break;
  case GOTO:
    sprintf(buf, "GOTO %d", p[(*pc)++]);
    break;
  case IFZERO:
    sprintf(buf, "IFZERO %d", p[*pc]);
    break;
  case IFNZRO:
    sprintf(buf, "IFNZRO %d", p[*pc]);
    break;
  case CALL:
    sprintf(buf, "CALL %d %d", p[(*pc)++], p[(*pc)++]);
    break;
  case TCALL:
    sprintf(buf, "TCALL %d %d %d", p[(*pc)++], p[(*pc)++], p[(*pc)++]);
    break;
  case RET:
    sprintf(buf, "RET %d", p[*pc]);
    break;
  case PRINTI:
    sprintf(buf, "PRINTI");
    break;
  case PRINTC:
    sprintf(buf, "PRINTC");
    break;
  case LDARGS:
    sprintf(buf, "LDARGS");
    break;
  case STOP:
    sprintf(buf, "STOP");
    break;
  default:
    sprintf(buf, "<unknown>");
    break;
  }
}

void printPrg(IntVec *insts) {
  int pc = 0;
  char ibuf[100];
  while (pc < insts->size) {
    readInst(insts->array, &pc, ibuf);
    printf("%d:\t%s\n", pc - 1, ibuf);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    exit(-1);
  }

  IntVec v;
  init_IntVec(&v, 0);

  for (int i = 1; i < argc; ++i) {
    int e = strtonum(argv[i], 0, INT_MAX, NULL);
    push_IntVec(&v, e);
  }

  /* print_IntVec(&v); */
  printPrg(&v);

  free_IntVec(&v);
}
