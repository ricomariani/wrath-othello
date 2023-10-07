#include "board.h"

int score(BOARD board, int is_white) {
  int i, s;
  unsigned int t;
  unsigned char *me, *him;

  me = &board[is_white][0];
  him = &board[!is_white][0];

  t = s = 0;
  if (turn > ENDGAME) {
    for (i = 0; i < 8; i++)
      s += bit_count[me[i]];
    return (s);
  }

  for (i = 2; i < 6; i++)
    s += weighted_row_value[me[i]];
  s -= bit_count[me[1] & 0x7e] + bit_count[me[6] & 0x7e] +
       ((bit_count[me[1] & 0x42] + bit_count[me[6] & 0x42]) << 2);

  for (i = 0; i < 8; i++)
    t = (t << 1) | (!!(me[i] & (1 << 7)));
  for (i = 0; i < 8; i++)
    t = (t << 1) | (!!(him[i] & (1 << 7)));

  s += edge[t];

  t = 0;
  for (i = 0; i < 8; i++)
    t = (t << 1) | (me[i] & 1);
  for (i = 0; i < 8; i++)
    t = (t << 1) | (him[i] & 1);

  s += edge[t] + edge[(me[0] << 8) | him[0]] + edge[(me[7] << 8) | him[7]];

  return (s);
}
