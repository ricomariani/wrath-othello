#include "board.h"

static uint16_t gethorz(uint8_t *me, uint8_t *him, int x, int y);
static void puthorz(uint8_t *me, uint8_t *him, int y, uint16_t row);
static uint16_t getvert(uint8_t *me, uint8_t *him, int x, int y);
static void putvert(uint8_t *me, uint8_t *him, int x, uint16_t row);
static uint16_t getdiag1(uint8_t *me, uint8_t *him, int x, int y);
static void putdiag1(uint8_t *me, uint8_t *him, int x, int y, uint16_t row);
static uint16_t getdiag2(uint8_t *me, uint8_t *him, int x, int y);
static void putdiag2(uint8_t *me, uint8_t *him, int x, int y, uint16_t row);

// This does the actually flipping needed if a piece is placed at x, y
// We do not validate that it is legal to do so, that has already
// happened.
void flip(BOARD board, int is_white, int x, int y) {
  uint16_t row, new;

  // normalize black/white to me/him as usual
  uint8_t *me = &board[is_white][0];
  uint8_t *him = &board[!is_white][0];

  // extract the bits for horizontal flipping and apply the flip table
  // Note that the flip table assumes the target square (x, y) is empty
  // so all the helpers normalize to empty.  We might be mid-flip
  // in the late phases and the "move" step already set the bits
  // (this could be changed by tweaking how we build flip_table)
  row = gethorz(me, him, x, y);
  new = flip_table[row][x];
  puthorz(me, him, y, new);

  // pull the vertical bits into a "row" and flip it, then put it back
  row = getvert(me, him, x, y);
  new = flip_table[row][y];

  // it's expensive to write vertical, so don't do it if nothing changed
  // remember we normalize back to "the square is filled by me" to do
  // this test.
  row |= (0x100 << y);
  if (new != row)
    putvert(me, him, x, new);

  // now the first diagonal direction (x and y both increasing)
  // same optimization
  row = getdiag1(me, him, x, y);
  new = flip_table[row][x];
  row |= (0x100 << x);
  if (new != row)
    putdiag1(me, him, x, y, new);

  // finally the second diagonal direction (x increasing and y decreasing)
  // same optimization
  row = getdiag2(me, him, x, y);
  new = flip_table[row][x];
  row |= (0x100 << x);
  if (new != row)
    putdiag2(me, him, x, y, new);
}

static uint16_t gethorz(uint8_t *me, uint8_t *him, int x, int y) {
  // pull the row out in the usual way and strip the "me" bit at column x
  // we don't have to normalize to x, y OFF because gethorz goes first
  // so the square at x, y has not yet been filled.  The other flips
  // have to assume that a previous direction might have already placed
  // the move piece at x, y
  return (me[y] << 8) | (him[y]);
}

static void puthorz(uint8_t *me, uint8_t *him, int y, uint16_t row) {
  // super easy to put horizontal row back
  me[y] = row >> 8;
  him[y] = row & 0xff;
}

static uint16_t getvert(uint8_t *me, uint8_t *him, int x, int y) {
  uint16_t row = ((him[0] >> x) & 1);
  row |= ((him[1] >> x) & 1) << 1;
  row |= ((him[2] >> x) & 1) << 2;
  row |= ((him[3] >> x) & 1) << 3;
  row |= ((him[4] >> x) & 1) << 4;
  row |= ((him[5] >> x) & 1) << 5;
  row |= ((him[6] >> x) & 1) << 6;
  row |= ((him[7] >> x) & 1) << 7;

  row |= ((me[0] >> x) & 1) << 8;
  row |= ((me[1] >> x) & 1) << 9;
  row |= ((me[2] >> x) & 1) << 10;
  row |= ((me[3] >> x) & 1) << 11;
  row |= ((me[4] >> x) & 1) << 12;
  row |= ((me[5] >> x) & 1) << 13;
  row |= ((me[6] >> x) & 1) << 14;
  row |= ((me[7] >> x) & 1) << 15;

  // normalize to x, y OFF
  row &= ~(0x100 << y);
  return row;
}

static void putvert(uint8_t *me, uint8_t *him, int x, uint16_t row) {
  // this time we will write out the x column so that it matches the
  // bits the row, reversing what getvert does.
  uint8_t mask_out = 1 << x;

  // flip the bits that need flipping
  him[0] ^= mask_out & (him[0] ^ ((row & 1) << x)); row >>= 1;
  him[1] ^= mask_out & (him[1] ^ ((row & 1) << x)); row >>= 1;
  him[2] ^= mask_out & (him[2] ^ ((row & 1) << x)); row >>= 1;
  him[3] ^= mask_out & (him[3] ^ ((row & 1) << x)); row >>= 1;
  him[4] ^= mask_out & (him[4] ^ ((row & 1) << x)); row >>= 1;
  him[5] ^= mask_out & (him[5] ^ ((row & 1) << x)); row >>= 1;
  him[6] ^= mask_out & (him[6] ^ ((row & 1) << x)); row >>= 1;
  him[7] ^= mask_out & (him[7] ^ ((row & 1) << x)); row >>= 1;

  me[0] ^= mask_out & (me[0] ^ ((row & 1) << x)); row >>= 1;
  me[1] ^= mask_out & (me[1] ^ ((row & 1) << x)); row >>= 1;
  me[2] ^= mask_out & (me[2] ^ ((row & 1) << x)); row >>= 1;
  me[3] ^= mask_out & (me[3] ^ ((row & 1) << x)); row >>= 1;
  me[4] ^= mask_out & (me[4] ^ ((row & 1) << x)); row >>= 1;
  me[5] ^= mask_out & (me[5] ^ ((row & 1) << x)); row >>= 1;
  me[6] ^= mask_out & (me[6] ^ ((row & 1) << x)); row >>= 1;
  me[7] ^= mask_out & (me[7] ^ ((row & 1) << x));
  
}

// get the first diagonal, this is where y goes up when x goes up
static uint16_t getdiag1(uint8_t *me, uint8_t *him, int x, int y) {
  int x0 = 0;
  int y0 = y - x;
  int y1 = 7;

  if (y0 < 0) {
    x0 = -y0;
    y1 = 7 - x0;
    y0 = 0;
  }

  uint8_t mask = 1 << x0;
  uint16_t row = 0;

  for (int i = y0; i <= y1; i++, mask <<= 1) {
    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= ((me[i] & mask) << 8) | (him[i] & mask);
  }

  // normalize to x, y OFF
  return row & ~(0x100 << x);
}

// write back the first diagonal, this is where y goes up when x goes up
static void putdiag1(uint8_t *me, uint8_t *him, int x, int y, uint16_t row) {
  int x0 = 0;
  int y0 = y - x;
  int y1 = 7;

  if (y0 < 0) {
    x0 = -y0;
    y1 = 7 - x0;
    y0 = 0;
  }

  uint8_t mask = 1 << x0;
  uint8_t hi = row >> 8;

  for (int i = y0; i <= y1; i++, mask <<= 1) {
    // flip the bits that need flipping
    me[i] ^= (hi ^ me[i]) & mask;
    him[i] ^= (row ^ him[i]) & mask;
  }
}

// get the second diagonal, this is where y goes down when x goes up
static uint16_t getdiag2(uint8_t *me, uint8_t *him, int x, int y) {
  int x0 = 0;
  int y0 = x + y;
  int y1 = 0;

  if (y0 > 7) {
     x0 = y0 - 7;
     y1 = x0;
     y0 = 7;
  }

  uint8_t mask = 1 << x0;
  uint16_t row = 0;

  for (int i = y0; i >= y1; i--, mask <<= 1) {
    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= ((me[i] & mask) << 8) | (him[i] & mask);
  }

  // normalize to x, y OFF
  return row & ~(0x100 << x);
}

// write back the second diagonal, this is where y goes down when x goes up
static void putdiag2(uint8_t *me, uint8_t *him, int x, int y, uint16_t row) {
  int x0 = 0;
  int y0 = x + y;
  int y1 = 0;

  if (y0 > 7) {
     x0 = y0 - 7;
     y1 = x0;
     y0 = 7;
  }

  uint8_t mask = 1 << x0;
  uint8_t hi = row >> 8;

  for (int i = y0; i >= y1; i--, mask <<= 1) {
    // flip the bit if needed
    me[i] ^= (hi ^ me[i]) & mask;
    him[i] ^= (row ^ him[i]) & mask;
  }
}
