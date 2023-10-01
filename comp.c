#include "board.h"
#include "endgame.h"
#include <stdio.h>
#include <stdlib.h>

void computer_input(BOARD board, int colour)
{
  int x, y, sc;

  sc = search(board, colour, &x, &y);
  if (x == -1) {
    printf("%c has to pass.\n", colour ? 'W' : 'B');
    fflush(stdout);
    consecutive_passes++;
  } else {
    printf("best move for %c is at %c%c (score %d)\n", colour ? 'W' : 'B',
           x + 'a', '8' - y, sc - ((turn > ENDGAME) ? 0 : (8187)));
    fflush(stdout);
    move(board, colour, x, y);
    consecutive_passes = 0;
  }
}
