#include "board.h"

// This is for keeping valid moves

typedef struct {
  char x;
  char y;
} xy;

// We keep moves we are considering here, this is for holding the next set of
// valid moves
typedef struct {
  char top;
  xy moves[64];
} stack;

// these are all the stacks we will ever need, no malloc
static stack stacks[64];

void reset_move_stack(int lvl) {
  stack *S = &stacks[lvl];
  S->top = 0;
}

// each next valid mmove at this recursion level is pushed on its own stack
void push(int x, int y, int lvl) {
  stack *S = &stacks[lvl];
  S->moves[S->top].x = x;
  S->moves[S->top].y = y;
  S->top++;
}

// and they come off... the order is arbitrary anyway and stack is cheap
// so we do that
int pop_move(int *x, int *y, int lvl) {
  stack *S = &stacks[lvl];
  if (S->top) {
    S->top--;
    *x = S->moves[S->top].x;
    *y = S->moves[S->top].y;
    return 1;
  }

  *x = *y = -1;
  return 0;
}
