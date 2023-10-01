#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

BOARD initial = {{0, 0, 0, 16, 8, 0, 0, 0}, {0, 0, 0, 8, 16, 0, 0, 0}};

char bit_count[256];
char weighted_row_value[256];
int turn;
int consecutive_passes;
int colour = 1;
void catch ();

int main(int argc, char **argv)
{
  int i, j;
  int player, play_side;
  FILE *f;

  setlocale(LC_ALL, "");

  player = 0;
  play_side = 0;
  if (argc >= 2) {
    switch (argv[1][0]) {

    case 'w':
    case 'W':
      player = 1;
      printf("You will be playing white.\n");
      fflush(stdout);
      play_side = 1;
      break;

    case 'b':
    case 'B':
      player = 1;
      printf("You will be playing black.\n");
      fflush(stdout);
      play_side = 0;
      break;
    }

    switch (argv[1][1]) {
    case 'l':
    case 'L':
      if (argc < 3) {
        fprintf(stderr, "wrath: You must specify a file name\n");
        fflush(stderr);
        exit(1);
      }
      load (argv[2], initial);
    }
  }

  printf("Give me about thirty seconds to build up my tables please\n");
  fflush(stdout);

  build_pack_table();
  for (i = 0; i < 256; i++) {
    for (j = 0; j < 8; j++) {
      // this is a straight bit count
      bit_count[i] += !!(i & (1 << j));

      // this doesn't count the edge slots and gives negative
      // value to the slots near the edge  so...
      //  0 -1 1 1 1 1 -1 0
      // the value of the row is the sum of the bit weights
      // for each bit that is set in that row.  So the
      // items near the end are not desireable.  The edges
      // get no weight because there is a seperate edge table
      // computation that determines the value of those cells.

      if (j > 1 && j < 6)
        weighted_row_value[i] += !!(i & (1 << j));

      if (j == 1 || j == 6)
        weighted_row_value[i] -= !!(i & (1 << j));
    }
  }

  // make the edge tables
  buildedge();

  // start with no passes
  consecutive_passes = 0;

  // display the initial board and score
  display(initial);
  display_score(initial);

  // repeat play until there are two passes, alternating color
  while (consecutive_passes < 2) {
    if (player && colour == play_side)
      user_input(initial, colour);
    else
      computer_input(initial, colour);

    display_score(initial);
    colour = !colour;
  }
}
