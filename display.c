#include "board.h"
#include <stdio.h>
#include <stdlib.h>

char a[] = "-BW?";
extern int turn;

#define INDEX(board, x, y) (!!((board)[y] & (1 << (x))))
#define RINDEX(board, x) (!!((board) & (1 << (x))))
#define TYPE(board, x, y)                                                      \
  (INDEX((board)[0], x, y) + (INDEX((board)[1], x, y) << 1))
#define RTYPE(board, x)                                                        \
  (RINDEX((board >> 8), x) + (RINDEX((board)&0xff, x) << 1))

display(board) BOARD board;
{
  register x, y;

  for (y = 0; y < 8; y++) {
    printf("\t\t\t\t%c ", '8' - y);
    for (x = 0; x < 8; x++) {
      putchar(a[TYPE(board, x, y)]);
      putchar(' ');
    }
    putchar('\n');
  }
  printf("\t\t\t\t  ");
  for (x = 0; x < 8; x++) {
    putchar('a' + x);
    putchar(' ');
  }
  putchar('\n');
  fflush(stdout);
}

rdisp(edge) {
  int x;

  for (x = 0; x < 8; x++)
    putchar(a[RTYPE(edge, x)]);
  fflush(stdout);
}

display_score(board) BOARD board;
{
  int sc1, sc2, i;

  sc1 = 0;
  for (i = 0; i < 8; i++)
    sc1 += val[board[1][i]];
  printf("\t\t\t\t\t\t    Score: W=%d ", sc1);
  sc2 = 0;
  for (i = 0; i < 8; i++)
    sc2 += val[board[0][i]];
  printf("B=%d\n", sc2);
  turn = sc1 + sc2;
  fflush(stdout);
}
