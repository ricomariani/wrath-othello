#include "board.h"
#include <stdio.h>
#include <stdlib.h>

char ascii_values[] = "-BW?";

// This is true or false if the bit is set in the row for one color
#define RINDEX(rowbits, x) (!!((rowbits) & (1 << (x))))

// this get the row out of the given board (black or white) and then is true
// if that board has a bit at x, y
#define INDEX(rows, x, y) RINDEX((rows)[y], x)

// this gets the type of the row board[0] is black and board[1] is white
#define TYPE(board, x, y) (INDEX((board)[0], x, y) + (INDEX((board)[1], x, y) << 1))

// here rowbits has a single row, black is the low bits and white is t he high bits
// the values are 0 empty, 1 black and 2 white just like the ascii table above
#define RTYPE(rowbits, x) (RINDEX((rowbits >> 8), x) + (RINDEX((rowbits)&0xff, x) << 1))


// draw all the rows of the board
void display(BOARD board)
{
  int  x, y;

  for (y = 0; y < 8; y++) {
    // row label (digit)
    printf("\t\t\t\t%c ", '8' - y);

    // board data
    for (x = 0; x < 8; x++) {
      putchar(ascii_values[TYPE(board, x, y)]);
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

void display_one_row(int rowbits) {
  int x;

  for (x = 0; x < 8; x++) {
    putchar(ascii_values[RTYPE(rowbits, x)]);
  }

  fflush(stdout);
}

// this is the raw score just counts
// the evaulation is not this at all.
void display_score(BOARD board)
{
  int sc1, sc2, i;

  sc1 = 0;
  for (i = 0; i < 8; i++)
    sc1 += bit_count[board[1][i]];
  printf("\t\t\t\t\t\t    Score: W=%d ", sc1);
  sc2 = 0;
  for (i = 0; i < 8; i++)
    sc2 += bit_count[board[0][i]];
  printf("B=%d\n", sc2);
  turn = sc1 + sc2;
  fflush(stdout);
}
