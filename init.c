#include "board.h"
#include <stdio.h>
#include <stdlib.h>

extern int colour;

void load(FILE *f, BOARD board)
{
  int i, j, n;
  char c[8];
  char s[80];

  for (i = 0; i < 8; i++)
    board[0][i] = board[1][i] = 0;

  for (i = 0; i < 8; i++) {
    fgets(s, 80, f);
    n = sscanf(s, " %c %c %c %c %c %c %c %c ", &c[0], &c[1], &c[2], &c[3],
               &c[4], &c[5], &c[6], &c[7]);
    if (n != 8) {
      fprintf(stderr, "wrath: Unable to parse input board\n");
      fflush(stderr);
      exit(1);
    }
    for (j = 0; j < 8; j++)
      switch (c[j]) {

      case 'B':
      case 'b':
        board[0][i] |= (1 << j);
        break;

      case 'W':
      case 'w':
        board[1][i] |= (1 << j);
        break;

      case '-':
        break;

      default:
        fprintf(stderr, "wrath: Board has invalid characters\n");
        fflush(stderr);
        exit(1);
      }
  }
  fscanf(f, " %c to play", c);
  switch (c[0]) {
  case 'w':
  case 'W':
    colour = 1;
    break;
  case 'b':
  case 'B':
    colour = 0;
    break;

  default:
    fprintf(stderr, "wrath: I can't tell whose turn is next\n");
    fflush(stderr);
    exit(1);
  }
  printf("Picking up where we left off... %s to play\n",
         colour ? "White" : "Black");
}
