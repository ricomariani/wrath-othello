#include "board.h"

BOARD initial = {{0, 0, 0, 16, 8, 0, 0, 0}, {0, 0, 0, 8, 16, 0, 0, 0}};

char bit_count[256];
char weighted_row_value[256];
int turn;
int consecutive_passes;
int is_white_turn = 1;
uint64_t bit_values[256];

static int test_mode = 0;

void printCompilerVersion() {
#if defined(__clang__) // Check for Clang
  printf("Compiler: Clang Version: %d.%d.%d\n", __clang_major__,
         __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__) // Check for GCC or Clang
  printf("Compiler: GCC (GNU C Compiler) Version: %d.%d.%d\n", __GNUC__,
         __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER) // Check for MSVC
  printf("Compiler: MSVC (Microsoft Visual C++) Version: %d\n", _MSC_VER);
#else
  printf("Compiler version detection not supported for this compiler.\n");
#endif
}

int main(int argc, char **argv) {
  int i, j;
  int player, play_side;
  FILE *f;

  // we print the framework version in managed code...
  printCompilerVersion();

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
      load(argv[2], initial);

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
    if (player && is_white_turn == play_side)
      user_input(initial, is_white_turn);
    else
      computer_input(initial, is_white_turn);

    display_score(initial);
    is_white_turn = !is_white_turn;

    if (test_mode) {
      printf("test complete\n");
      exit(0);
    }
  }
}
