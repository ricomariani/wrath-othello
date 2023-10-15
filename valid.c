#include "board.h"

// the current depth just tell us which stack to put the valid moves on
// the stacks are all pre-allocated so there is no malloc
int valid(BOARD board, int is_white, int current_depth) {
  uint8_t *me = &board[is_white][0];
  uint8_t *him = &board[!is_white][0];

  reset_move_stack(current_depth);
  int found_anything = 0;

  for (uint8_t y = 0; y < 8; y++) {
    ushort row = (ushort)((me[y] << 8) | him[y]);
    uint8_t used = (uint8_t)(row | (row >> 8));

    // already full on this row, nothing to do
    if (used == 0xff)
      continue;

    uint8_t initial_used = used;
    uint8_t mask = 1;
    for (uint8_t i = 0; i < 8; i++, mask <<= 1) {
      // if the current spot is occupied, skip it, no valid move here
      if ((initial_used & mask) != 0)
        continue;

      // flip_table[base_3_row_index][i] tells you what the state is of the
      // row if you were to place a piece at column i.  So this says
      // if the effect of placing a piece at column i is only that the
      // piece at column i appears, then nothing flips, so it's not valid.
      // remember 'me' is in the high bits and 'him' is in the low bits
      // So we place onto the high bits, hence mask << 8.

      if ((row | (mask << 8)) != flip_table[row][i]) {
        push_move(i, y, current_depth);
        used |= mask;
        found_anything = 1;
      }
    }

    // if we found valid moves everywhere on this row nothing further to do
    // there couldn't be any more valid moves anyway.
    if (used == 0xff)
      continue;

    // We think of the matching as a regular expression
    // and build the state machine to match it. If E is enemy, B
    // is blank and M is mine then the match pattern is BH+M.
    //
    // Starting empty (state 0) then finding enemy piece (state 1)
    // the a piece of mine (state 3 if found), once the match fails
    // we go to state 2 and stay there. Actually we speed this up
    // by starting in state 2 if the current spot is occupied and
    // then only reading rows from there.  The machine is below:
    //
    // input alphabet:
    //
    // blank  = 0
    // enemy  = 1
    // me     = 2
    // ignore = 3  this doesn't happen it's empty, black, or white, not both
    //
    // state transitions:
    //
    // X indicates don't care because it can't happen
    // so we can pick whatever is convenient
    //
    // state is represented in two bits y0 (lo) and y1 (hi)
    // alpha is represented in two bits d0 (lo) and d1 (hi)
    //
    //                  h m
    //          input 0 1 2 3   00 01 10 x
    // state 0:       2 1 2 X   10 01 10 x
    // state 1:       3 1 2 X   11 01 10 x
    // state 2:       2 2 2 X   10 10 10 x
    // state 3:       3 3 3 X   11 11 11 x  VALID MOVE END STATE

    // y0 transitions all bits in karnaugh map order
    //                          BEST FREE CHOICE
    //           00 01 11 10    00 01 11 10
    // state 00:  0  1  x  0     0  1  1  0
    // state 01:  1  1  x  0     1  1  1  0
    // state 11:  1  1  x  1     1  1  1  1 VALID MOVE END STATE
    // state 10:  0  0  x  0     0  0  0  0
    //
    // we see that we can cover the true cases with:
    //  a row  y0 & y1
    //  a box  y0 & d1
    //       these two are compose to y0 & (y1|d1)
    //  a box  ~y1 & d0
    //
    //  net expression:  y0 = (~y1 & d0) | (y0 & (y1|d1))
    //
    // y1 transitions all bits in karnaugh map order
    //
    //                         BEST FREE CHOICE
    //           00 01 11 10    00 01 11 10
    // state 00:  1  0  x  1     1  0  1  1
    // state 01:  1  0  x  1     1  0  1  1
    // state 11:  1  1  x  1     1  1  1  1  VALID MOVE END STATE
    // state 10:  1  1  x  1     1  1  1  1
    //
    // we can cover those cases with
    // bottom half : y1
    // right  half : d1
    // first and last column:  ~d0 (the old wrap around trick)
    //
    // net expression y1 = y1 | d1 | ~d0
    // i.e.  y1 |= d1 | ~d0
    //
    // Those are exactly the binary operations below to run the state machine
    //
    // To find the valid bits, those are state3 or 11 so we just do y0 &= y1
    // any bits that are set are valid moves.
    //
    // I'm writing this comment 36 years after I wrote this code
    // and I'm stunned that it worked out on the first go...

    // all used bits start in state 2 -> no match found
    // we never leave state 2 (see above)
    uint32_t y0 = 0;
    uint32_t y1 = used | (used << 8) | (used << 16);

    for (int i = y - 1; i >= 0; i--) {
      uint8_t h = him[i];
      uint8_t m = me[i];
      uint32_t d0 = ((h >> (y - i)) << 16) | ((h << (y - i)) << 8) | h;
      uint32_t d1 = ((m >> (y - i)) << 16) | ((m << (y - i)) << 8) | m;

      y0 = (~y1 & d0) | (y0 & (y1 | d1));
      y1 |= d1 | ~d0;
      if (0 == ~y1)
        break;
    }

    y0 &= y1;
    y0 |= y0 >> 8;
    y0 |= y0 >> 8;
    uint8_t found = y0;
    uint8_t used2 = used | found;

    y1 = used2 | (used2 << 8) | (used2 << 16);
    y0 = 0;
    for (int i = y + 1; i < 8; i++) {
      uint8_t h = him[i];
      uint8_t m = me[i];
      uint32_t d0 =
          ((uint8_t)(h >> (i - y)) << 16) | ((uint8_t)(h << (i - y)) << 8) | h;
      uint32_t d1 =
          ((uint8_t)(m >> (i - y)) << 16) | ((uint8_t)(m << (i - y)) << 8) | m;

      y0 = (~y1 & d0) | (y0 & (y1 | d1));
      y1 |= d1 | ~d0;
      if (0 == ~y1)
        break;
    }

    y0 &= y1;
    y0 |= y0 >> 8;
    y0 |= y0 >> 8;
    found |= y0;
    found &= ~used;

    // bit_values has one bit number in each nibble
    // this saves us from looking for all 8 bits
    // when often there is only 1 bit set
    if (found != 0) {
      ulong bits = bit_values[found];
      while (bits != 0) {
        uint8_t x = (uint8_t)(bits & 0x7);
        bits >>= 4;
        push_move(x, y, current_depth);
        found_anything = 1;
      }
    }
  }

  return found_anything;
}
