#include "board.h"

// the computer provides "input" for the next move
// it's done uniformly like this so that the logic for
// player vs. computer is basically the same as computer
// vs. itself.
void computer_input(BOARD board, int colour)
{
  int x, y;

  int score = search(board, colour, &x, &y);
  if (x == -1) {
    printf("%c has to pass.\n", colour ? 'W' : 'B');
    fflush(stdout);
    consecutive_passes++;
  } else {
    // at the endgame the score is the number of pieces we have
    int score_bias = turn > ENDGAME ? 0 : SCORE_BIAS;
    printf("best move for %c is at %c%c (score %d)\n", colour ? 'W' : 'B',
           x + 'a', '8' - y, score - score_bias);
    fflush(stdout);
    move(board, colour, x, y);
    consecutive_passes = 0;
  }
}
