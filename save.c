#include "board.h"

// I grew up in a world where you could call gets and it was ok
void safe_gets(char *buf, int len) {
  char *result = fgets(buf, len, stdin);
  if (result) {
    // clobber the trailing \n
    buf[strlen(buf) - 1] = 0;
  } else {
    buf[0] = 0;
  }
}

// Produces this:
//
// - - B B - W - -
// B - B B B B - B
// B B W W W W B B
// B W B W W B W B
// - W W B B W W -
// W W W W W W W W
// - - - W B B - -
// - - W - B B - -
//
// b to play
//
// Which is useful because it's super easy to make one by hand
// The "load" function reads this same format.
// Returns true if the save aborted.
int save() {
  char name[80];

  printf("filename (press return to abort): ");
  fflush(stdout);
  safe_gets(name, sizeof(name));
  if (!name[0])
    return (1);

  FILE *f = fopen(name, "w");
  if (!f)
    return (1);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      putc(BoardCharAt(initial, x, y), f);
      putc(' ', f);
    }
    fputc('\n', f);
  }
  putc('\n', f);
  fprintf(f, "%c to play\n", colour ? 'w' : 'b');
  fclose(f);
  return 0;
}

// once used as the ^C handler
// it isn't wired in anymore but it could be
static void catch () {
  char confirm[100];

  printf("\nreally quit (y/n)? ");
  fflush(stdout);
  safe_gets(confirm, sizeof(confirm));
  if (confirm[0] != 'y')
    return;

  printf("save game (y/n)? ");
  fflush(stdout);
  safe_gets(confirm, sizeof(confirm));
  if (!confirm[0])
    return;

  if (confirm[0] == 'y' && save())
    return;

  exit(0);
}
