#include "board.h"

BOARD initial = {{0, 0, 0, 16, 8, 0, 0, 0}, {0, 0, 0, 8, 16, 0, 0, 0}};

char bit_count[256];
char weighted_row_value[256];
int turn;
int consecutive_passes;
int colour = 1;
uint64_t bit_values[256];

static int test_mode = 0;

int main(int argc, char **argv)
{
  int i, j;
  int player, play_side;
  FILE *f;

  // LOL, it was 30 seconds on a slice of a VAX780 :D
  printf("Give me about thirty seconds to build up my tables please\n");
  fflush(stdout);

  // make the lookup tables
  build_tables();

  // start with no passes
  consecutive_passes = 0;

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

      if (argv[1][2] == 't' | argv[1][2] == 'T') {
         // do one move and stop
         test_mode = 1;
      }
    }
  }

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

    if (test_mode) {
      printf("test complete\n");
      exit(0);
    }
  }
}
