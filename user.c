#include "board.h"

#define INITIAL_DEPTH 0

// here we ask the user what they want to do.
int user_input(BOARD board, int is_white) {
again:;
  // user input x and y
  int user_x = -1;
  int user_y = -1;
  int user_pass = 0;

  reset_move_stack(INITIAL_DEPTH);

  // recompute the valid moves and put them on the stack
  // they go on stack number INITIAL_DEPTH (i.e. the root)
  uint8_t *me = &board[is_white][0];
  uint8_t *him = &board[!is_white][0];
  valid(me, him, INITIAL_DEPTH);

  printf("Please enter a move --> ");
  fflush(stdout);

  char s[80];
  safe_gets(s, sizeof(s));

  int len = strlen(s);

  user_pass = 0;

  for (int i = 0; i < len; i++) {
    if (s[i] == '?') {
      // help
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
    if (s[i] >= 'a' && s[i] <= 'h') {
      // column
      user_x = s[i] - 'a';
    }
    if (s[i] >= '1' && s[i] <= '8') {
      // row
      user_y = '8' - s[i];
    }
    if (s[i] == 'p') {
      // pass
      user_pass = 1;
    }
    if (s[i] == 'q') {
      // quit
      exit(0);
    }
    if (s[i] == 'r') {
      // redraw the display
      display(board);
      display_score(board);
      goto again;
    }
    if (s[i] == 's') {
      if (save()) {
        goto again;
      } else {
        exit(0);
      }
    }
  }

  if ((user_x == -1 || user_y == -1) && user_pass == 0) {
    printf("?syntax error\n");
    fflush(stdout);
    goto again;
  }

  if (user_pass == 1) {
    // make sure there are no moves
    int x, y;
    if (pop_move(&x, &y, INITIAL_DEPTH)) {
      printf("You can't pass unless you have no moves\n");
      fflush(stdout);
      goto again;
    }
    consecutive_passes++;
    printf("Pass accepted.\n");
    fflush(stdout);
    return 0;
  }

  // make sure that the entered move is a valid move
  int x, y;
  while (pop_move(&x, &y, INITIAL_DEPTH)) {
    if (x == user_x && y == user_y) {
      printf("Move to %c%c accepted.\n", x + 'a', '8' - y);
      fflush(stdout);
      move(board, is_white, x, y);
      consecutive_passes = 0;
      return 0;
    }
  }
  printf("You can't move to %c%c.\n", user_x + 'a', '8' - user_y);
  fflush(stdout);
  goto again;
}
