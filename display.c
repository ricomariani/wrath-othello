#include "board.h"

char ascii_values[] = "-BW?";

// this gets the type of the row board[0] is black and board[1] is white
// this is used only for display
char BoardCharAt(BOARD board, int x, int y) {
  int i1 = (board[0][y] >> x) & 1;
  int i2 = (board[1][y] >> x) & 1;
  return ascii_values[i1 + (i2 << 1)];
}

// here row has a single row, black is the high bits and white is the low bits
// the values are 0 empty, 1 black and 2 white just like the ascii table above
// this is used only for display
char RowCharAt(int row, int x) {
  return ascii_values[((row >> (8 + x)) & 1) + 2 * ((row >> x) & 1)];
}

// draw all the rows of the board
void display(BOARD board) {
  int x, y;

  for (y = 0; y < 8; y++) {
    // row label (digit)
    printf("\t\t\t\t%c ", '8' - y);

    // board data
    for (x = 0; x < 8; x++) {
      putchar(BoardCharAt(board, x, y));
      putchar(' ');
    }
    putchar('\n');
  }
  printf("\t\t\t\t  ");

  // column label (letter)
  for (x = 0; x < 8; x++) {
    putchar('a' + x);
    putchar(' ');
  }
  putchar('\n');
  fflush(stdout);
}

// this is only used for debugging
void display_one_row(int row) {
  for (int x = 0; x < 8; x++) {
    putchar(RowCharAt(row, x));
  }

  fflush(stdout);
}

// this is the raw score, just counts
// the evaulation is not this at all.
void display_score(BOARD board) {
  int sc1, sc2, i;

  sc1 = 0;
  for (i = 0; i < 8; i++) {
    sc1 += bit_count[board[1][i]];
  }

  printf("\t\t\t\t\t\t    Score: W=%d ", sc1);

  sc2 = 0;
  for (i = 0; i < 8; i++) {
    sc2 += bit_count[board[0][i]];
  }

  printf("B=%d\n", sc2);
  turn = sc1 + sc2;
  fflush(stdout);
}
