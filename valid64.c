#include <memory.h>
#include <stdint.h>
#include "board.h"

#define tobyte(x) ((unsigned char)(x))
#define load_state(a, b, c) ((tobyte(a) << 16) | (tobyte(b) << 8) | tobyte(c))
#define unload_state(a) tobyte(((a) >> 16) | ((a) >> 8) | (a))

// the current depth just tell us which stack to put the valid moves on
// the stacks are all pre-allocated so there is no malloc
int valid(BOARD board, int colour, int current_depth)
{
  // words[0], [2] and [4] will always be zero.  They give a padding of empty board
  // rows around the real boards at [1] and [3]. This lets us go off the top and bottom
  // of the board with no bad consequences as we always hit empty rows.  As a
  // result the loops can be simpler.
  uint64_t words[5];
  words[0] = 0;
  words[1] = *(uint64_t *)&board[colour][0];
  words[2] = 0;
  words[3] = *(uint64_t *)&board[!colour][0];
  words[4] = 0;

  uint8_t *me  = (uint8_t*)(&words[1]);
  uint8_t *him = (uint8_t*)(&words[3]);

  reset_move_stack(current_depth);
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
        push(i, y, current_depth);
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

    uint64_t y0 = 0;
    uint64_t y1 = load_state(used, used, used);
    y1 |= y1 << 32;

    for (int i = 1; i < 8; i++) {
      int up = y + i;   // we can go off the end
      int down = y - i;

      uint64_t up_d0, up_d1, down_d0, down_d1, d;

      d = him[up];     up_d0 = load_state(d >> i, d << i, d);
      d = me[up];      up_d1 = load_state(d >> i, d << i, d);
      d = him[down]; down_d0 = load_state(d >> i, d << i, d);
      d = me[down];  down_d1 = load_state(d >> i, d << i, d);

      uint64_t d0 = (up_d0 << 32) | down_d0;
      uint64_t d1 = (up_d1 << 32) | down_d1;

      // state machine logic see above
      y0 = ((~y1) & d0) | (y0 & (y1 | d1));
      y1 |= d1 | (~d0);

      // when y1 is set the computation is finished either way, if they are all finished
      // then we can bail out.
      if ((~y1) == 0)
        break;
    }
    // read out: state 3 is valid move
    y0 &= y1;
    y0 |= (y0 >> 32);
    row = unload_state(y0);  // merge the successes from the 3 directions

    row &= ~(used);

    // now if anything was found, log it
    if (row) {
      for (int i = 0; i < 8; i++) {
        if (row & (1 << i)) {
          push(i, y, current_depth);
          found_anything = 1;
        }
      }
    }
  }
  return found_anything;
}
