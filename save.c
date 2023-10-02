#include "board.h"

#define INDEX(board, x, y) (!!((board)[y] & (1 << (x))))
#define RINDEX(board, x) (!!((board) & (1 << (x))))
#define TYPE(board, x, y)                                                      \
  (INDEX((board)[0], x, y) + (INDEX((board)[1], x, y) << 1))
#define RTYPE(board, x)                                                        \
  (RINDEX((board >> 8), x) + (RINDEX((board)&0xff, x) << 1))

void safe_gets(char *buf, int len)
{
  char *result = fgets(buf, len, stdin);
  if (result) {
    // clobber the trailing \n
    buf[strlen(buf) - 1] = 0;
  } else {
    buf[0] = 0;
  }
}

int save()
{
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
      putc(ascii_values[TYPE(initial, x, y)], f);
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
static void catch ()
{
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
