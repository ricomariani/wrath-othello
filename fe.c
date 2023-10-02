#include "board.h"

#define DATA(mask, x) ((mask) & (1 << (x)))

unsigned fe(unsigned mask, int colour, int x, int dx)
{
  int i, him, me, x0;

  him = mask & 0xff;
  me = mask >> 8;
  x0 = x;

  if (colour) {
    i = him;
    him = me;
    me = i;
  }

  if (!DATA(him, x + dx))
    goto done;

  for (i = 0; i < 8; i++) {
    x += dx;
    if (x < 0 || x > 7)
      goto done;
    if (DATA(me, x)) {
      x -= dx;
      while (x != x0) {
        me |= (1 << x);
        him &= ~(1 << x);
        x -= dx;
      }
      goto done;
    }
    if (!DATA(him, x))
      goto done;
  }
done:
  me |= (1 << x0);
  if (colour) {
    i = him;
    him = me;
    me = i;
  }
  return ((me << 8) | him);
}
