#include "board.h"
#include "endgame.h"
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HORRIBLE -32000
#define GREAT 32000

extern int turn;
int boards;
jmp_buf env;
int totaltime;
int starttime;
int thistime;
static searching_to_end;
int bx, by;
int bs;

static limit;
int IRQ;

static void bcpy(BOARD b1, BOARD b2);

void timeout() { IRQ = 1; }

static void print_with_commas(int n) {
  if (n < 1000) {
    printf ("%d", n);
  }
  else {
    print_with_commas(n / 1000);
    printf(",%03d", n % 1000);
  }
}


int search(BOARD board, int colour, int *bestx, int *besty)
{
  int i, start, lvl, moves;

  signal(SIGALRM, timeout);

  if (turn < 15)
    limit = 2;
  else if (turn < 30)
    limit = 4;
  else
    limit = 10;

  boards = 0;
  searching_to_end = 0;
  IRQ = 0;

  starttime = time(0);
  if (turn <= ENDGAME) {
    alarm(limit);
    start = 2;
  } else {
    printf("Seeking to end of board\n");
    fflush(stdout);
    start = 64 - turn;
    if (start % 2)
      start++;
  }

  if (setjmp(env)) {
    *bestx = bx;
    *besty = by;
    thistime = time(0) - starttime;
    if (!thistime)
      thistime = 1;

    totaltime += thistime;

    printf("\nEvaluated ");
    print_with_commas(boards);
    printf(" boards in %d:%02d (", thistime / 60, thistime % 60);
    print_with_commas(boards / thistime);
    printf(" boards/sec). ");
    printf("Total time used=%d:%02d \n", totaltime / 60, totaltime % 60);
    fflush(stdout);

    return bs;
  }

  if (!(moves = valid(board, colour, start))) {
    bx = -1;
    by = -1;
    bs = HORRIBLE;
    goto no_moves;
  }

  reset_scored_moves(0);
  while (pop_move(bestx, besty, start)) {
    insert_scored_move(*bestx, *besty, 0, 0);
  }

  lvl = 0;
  for (i = start; i < 65; i += 2) {
    printf("%2d: ", i);
    fflush(stdout);
    rsearch(board, colour, i, lvl);
    if (i + 2 > 64 - turn)
      break;
    lvl = !lvl;
  }

no_moves:
  alarm(0);
  longjmp(env, 1);
}

int rsearch(BOARD board, int colour, int depth, int lvl)
{
  int x, y, sc;
  BOARD brd;
  int moves;

  reset_scored_moves(!lvl);
  moves = 0;
  searching_to_end = 0;

  if (!remove_scored_move(&x, &y, lvl))
    return 0;

  printf("%c%c=", x + 'a', '8' - y);
  fflush(stdout);
  bcpy(brd, board);
  flip(brd, colour, x, y);

  bs = mini(brd, !colour, depth - 1, HORRIBLE, GREAT);
  bx = x;
  by = y;
  printf("%d  ", bs - ((turn > ENDGAME) ? 0 : 8187));
  fflush(stdout);
  insert_scored_move(x, y, bs, !lvl);

  while (remove_scored_move(&x, &y, lvl)) {
    printf("%c%c", x + 'a', '8' - y);
    fflush(stdout);
    bcpy(brd, board);
    flip(brd, colour, x, y);
    sc = mini(brd, !colour, depth - 1, bs, GREAT);
    insert_scored_move(x, y, sc, !lvl);
    if (sc > bs) {
      putchar('=');
      bx = x;
      by = y;
      bs = sc;
    } else
      putchar('<');

    printf("%d  ", sc - ((turn > ENDGAME) ? 0 : 8187));
    fflush(stdout);
  }
  printf("\n");
  fflush(stdout);
  return bs;
}

int mini(BOARD board, int colour, int depth, int a, int b)
{
  if (IRQ)
    longjmp(env, 1);
  boards++;
  if (!depth)
    return score(board, colour);
  else {
    BOARD brd;
    int x, y, sc, yes;

    reset_move_stack(depth);

    yes = valid(board, colour, depth);
    if (!yes) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = 1;
        sc = maxi(board, !colour, depth + 1, a, b);
      } else
        sc = maxi(board, !colour, depth - 1, a, b);
      return sc;
    }
    searching_to_end = 0;

    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, colour, x, y);
      sc = maxi(brd, !colour, depth - 1, a, b);
      if (sc < b) {
        b = sc;
        if (b <= a)
          return b;
      }
    }
    return b;
  }
}

int maxi(BOARD board, int colour, int depth, int a, int b)
{
  if (IRQ)
    longjmp(env, 1);
  boards++;
  if (!depth)
    return score(board, colour);
  else {
    BOARD brd;
    int x, y, sc, yes;

    reset_move_stack(depth);

    yes = valid(board, colour, depth);
    if (!yes) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = 1;
        sc = mini(board, !colour, depth + 1, a, b);
      } else
        sc = mini(board, !colour, depth - 1, a, b);
      return sc;
    }
    searching_to_end = 0;

    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, colour, x, y);
      sc = mini(brd, !colour, depth - 1, a, b);
      if (sc > a) {
        a = sc;
        if (a >= b)
          return a;
      }
    }
    return a;
  }
}

static void bcpy(BOARD b1, BOARD b2)
{
  memcpy(b1, b2, sizeof(BOARD));
}

void move(BOARD board, int colour, int x, int y)
{
  board[colour][y] |= (1 << x);
  flip(board, colour, x, y);
  display(board);
}

void show(int depth, int score) {
  int i;

  for (i = 0; i < depth; i++)
    putchar('\t');

  printf("%d\n", score);
  fflush(stdout);
}
