#include "board.h"

#define stuff(a, b, c)                                                         \
  (((((unsigned long)a) & 0xff) << 16) | ((((unsigned long)b) & 0xff) << 8) |  \
   ((c)&0xff))
#define unstuff(a) ((((a) >> 16) | ((a) >> 8) | (a)) & 0xff)

int valid(BOARD board, int colour, int stack)
{
  unsigned char *me = &board[colour][0];
  unsigned char *him = &board[!colour][0];

  reset_move_stack(stack);
  int found_anything = 0;

  for (int y = 0; y < 8; y++) {
    unsigned row = (me[y] << 8) | him[y];
    unsigned char used = (row | (row >> 8));

    // already full on this row, nothing to do
    if (used == 0xff)
      continue;

    unsigned initial_used = used;
    unsigned mask = 1;
    for (int i = 0; i < 8; i++, mask <<= 1) {
      // if the current spot is occupied, skip it, no valid move here
      if (initial_used & mask)
        continue;

      // flipt[i][base_3_row_index] tells you what the state is of the
      // row if you were to place a piece at column i.  So this says
      // if the effect of placing a piece at column i is only that the
      // piece at column i appears, then nothing flips, so it's not valid.
      // remember me is in the high bits and him is in the low bits
      // so we place onto the high bits.  And d1 has the current bit mask
      if ((row | (mask << 8)) != flipt[i][pack_table[row]]) {
        push(i, y, stack);
        used |= mask;
        found_anything = 1;
      }
    }

    // if we found valid moves everywhere on this row nothing further to do
    // there couldn't be any more valid moves anyway.
    if (used == 0xff)
      continue;

    unsigned int y0, y1, d0, d1;

    y1 = stuff(used, used, used);
    y0 = 0;
    for (int i = y - 1; i >= 0; i--) {
      d0 = stuff(him[i] >> (y - i), him[i] << (y - i), him[i]);
      d1 = stuff(me[i] >> (y - i), me[i] << (y - i), me[i]);
      y0 = ((~y1) & d0) | (y0 & (y1 | d1));
      y1 |= d1 | (~d0);
      if ((~y1) == 0)
        break;
    }
    y0 &= y1;
    row = unstuff(y0);

    y1 = stuff(used | row, used | row, used | row);
    y0 = 0;
    for (int i = y + 1; i < 8; i++) {
      d0 = stuff(him[i] >> (i - y), him[i] << (i - y), him[i]);
      d1 = stuff(me[i] >> (i - y), me[i] << (i - y), me[i]);
      y0 = ((~y1) & d0) | (y0 & (y1 | d1));
      y1 |= d1 | (~d0);
      if ((~y1) == 0)
        break;
    }

    y0 &= y1;
    row |= unstuff(y0);
    row &= ~(used);
    used |= row;
    if (row) {
      for (int i = 0; i < 8; i++) {
        if (row & (1 << i)) {
          push(i, y, stack);
          found_anything = 1;
        }
      }
    }
  }
  return found_anything;
}
