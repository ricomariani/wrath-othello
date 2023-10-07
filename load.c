#include "board.h"

// When I rewrote this in C# I did something much
// simpler than the original C.  I back-ported
// the C# to C.  The orignal thing used fscanf
// and stuff which was total overkill and made it
// uglier.
void load(const char *name, BOARD board) {
  FILE *f = fopen(name, "r");
  if (!f) {
    fprintf(stderr, "wrath: I can't open this file: '%s'\n", name);
    fflush(stderr);
    exit(1);
  }

  for (int i = 0; i < 8; i++) {
    board[WHITE][i] = 0;
    board[BLACK][i] = 0;

    for (int j = 0; j < 8; ) {
      int ch = fgetc(f);
      if (ch == EOF) {
        printf("wrath: Unable to parse input board\n");
        exit(1);
      }
  
      if (isspace((unsigned char)ch)) continue;

      switch (ch) {

      case 'B':
      case 'b':
        board[BLACK][i] |= 1<<j;
        break;

      case 'W':
      case 'w':
        board[WHITE][i] |= 1<<j;
        break;

      case '-':
        break;

      default:
        printf("wrath: Board has invalid characters\n");
        exit(1);
        break;
      }

      j++;
    }
  }

  for (;;) {
    int ch = fgetc(f);
    if (ch == EOF) {
      printf("wrath: Unable to parse input board\n");
      exit(1);
    }

    if (isspace((unsigned char)ch)) continue;

    switch (ch) {
    case 'w':
    case 'W':
      is_white_turn = 1;
      break;

    case 'b':
    case 'B':
      is_white_turn = 0;
      break;

    default:
      printf("wrath: I can't tell whose turn is next\n");
      exit(1);
      break;
    }

    printf("Picking up where we left off... %s to play\n",
          is_white_turn ? "White" : "Black");
    break;
  }
}
