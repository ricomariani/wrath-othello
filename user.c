
#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK 0

void safe_gets(char *, int);

user_input(board, colour) BOARD board;
{
  char s[80];
  int p, ux, uy, x, y, i, l;

again:
  new (STACK);
  while (pop(&x, &y, STACK))
    ;

  valid(board, colour, STACK);

  printf("Please enter a move --> ");
  fflush(stdout);
  safe_gets(s, sizeof(s));

  l = strlen(s);
  uy = ux = -1;
  p = 0;

  for (i = 0; i < l; i++) {
    if (s[i] == '?') {
      printf("\n");
      printf("?\t\t\t\t:display this help page\n");
      printf("[a-z][1-8]\t\t\t:enter a move\n");
      printf("p\t\t\t\t:pass this turn\n");
      printf("r\t\t\t\t:redraw the board\n");
      printf("s\t\t\t\t:save current game\n");
      printf("q\t\t\t\t:quit current game\n");
      printf("\n");
      fflush(stdout);
      goto again;
    }
    if (s[i] >= 'a' && s[i] <= 'h')
      ux = s[i] - 'a';
    if (s[i] >= '1' && s[i] <= '8')
      uy = '8' - s[i];
    if (s[i] == 'p')
      p = 1;
    if (s[i] == 'q')
      exit(0);
    if (s[i] == 'r') {
      display(board);
      display_score(board);
      goto again;
    }
    if (s[i] == 's')
      if (save())
        goto again;
      else
        exit(0);
  }

  if ((ux == -1 || uy == -1) && p == 0) {
    printf("?syntax error\n");
    fflush(stdout);
    goto again;
  }

  if (p == 1) {
    if (pop(&x, &y, STACK)) {
      printf("You can't pass unless you have no moves\n");
      fflush(stdout);
      goto again;
    }
    consecutive_passes++;
    printf("Pass accepted.\n");
    fflush(stdout);
    return (0);
  }

  while (pop(&x, &y, STACK)) {
    if (x == ux && y == uy) {
      printf("Move to %c%c accepted.\n", x + 'a', '8' - y);
      fflush(stdout);
      move(board, colour, x, y);
      consecutive_passes = 0;
      return (0);
    }
  }
  printf("You can't move to %c%c.\n", ux + 'a', '8' - uy);
  fflush(stdout);
  goto again;
}
