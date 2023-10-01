#include <stdio.h>
#include <stdlib.h>

#include "board.h"

static void safe_fgets(char *s, int len, FILE *f) {
  char *result = fgets(s, len, f);
  if (!result) {
    fprintf(stderr, "wrath: Unable to parse input board\n");
    fflush(stderr);
    exit(1);
  }
}

void load(const char *name, BOARD board)
{
  int i, j, n;
  char c[8];
  char s[80];

  FILE *f = fopen(name, "r");
  if (!f) {
    fprintf(stderr, "wrath: I can't open this file: '%s'\n", name);
    fflush(stderr);
    exit(1);
  }

  for (i = 0; i < 8; i++)
    board[0][i] = board[1][i] = 0;

  for (i = 0; i < 8; i++) {
    safe_fgets(s, sizeof(s), f);
    
    n = sscanf(s, " %c %c %c %c %c %c %c %c ", 
      &c[0], &c[1], &c[2], &c[3],
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

  safe_fgets(s, sizeof(s), f);
  safe_fgets(s, sizeof(s), f);
  n = sscanf(s, " %c to play", c);
  if (n != 1) {
    fprintf(stderr, "wrath: Unable to parse input board\n");
    fflush(stderr);
    exit(1);
  }
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
