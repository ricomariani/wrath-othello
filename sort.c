#include "board.h"

// This is keeping sorted scores, there are two sets of scores

typedef struct {
  int score;
  char x;
  char y;
} scored_move;

// note that this list is small, like if there are 10 valid scored_moves that's
// a lot the size is 64 because that's how many squares there are on the board
// and that's still small but it we can't really have 64 valid scored_moves
typedef struct {
  char put; // the number we put in
  char get; // the one to get next
  scored_move scored_moves[64];
} scored_move_list;

static scored_move_list slist[2];

// reset the count of scored_moves in this level
// lvl is 0/1 corresponding to the current recursion level, it alternates
// so we're reading off of lvl and writing onto !lvl at any moment
void reset_scored_moves(int lvl) {
  scored_move_list *S = &slist[lvl];
  S->get = S->put = 0;
}

void insert_scored_move(int x, int y, int score, int lvl) {
  int i, j;
  scored_move_list *S = &slist[lvl];

  // find the place to insert this scored_move
  // stop at the first place where this score is bigger
  // (i.e. the best ends up at the front)
  for (i = 0; i < S->put; i++)
    if (score > S->scored_moves[i].score)
      break;

  if (i < S->put) {
    // bulk scored_move the scored_moves above this one to make room for the new
    // scored_move
    memmove(&S->scored_moves[i + 1], &S->scored_moves[i],
            sizeof(scored_move) * (S->put - i));
  }

  scored_move m = {.x = x, .y = y, .score = score};

  S->scored_moves[i] = m;
  S->put++;
}

int remove_scored_move(int *x, int *y, int lvl) {
  scored_move_list *S = &slist[lvl];

  if (S->get >= S->put) {
    *x = *y = -1;
    return 0;
  }

  *x = S->scored_moves[S->get].x;
  *y = S->scored_moves[S->get].y;
  S->get++;

  return 1;
}
