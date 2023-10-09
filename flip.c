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
  // normalize to x, y OFF
  return (me[y] << 8) | (him[y]) & ~(0x100 << x);
}

static void puthorz(uint8_t *me, uint8_t *him, int y, uint16_t row) {
  // super easy to put horizontal row back
  me[y] = row >> 8;
  him[y] = row & 0xff;
}

static uint16_t getvert(uint8_t *me, uint8_t *him, int x, int y) {
  // we're going to read out this column in me and him
  int8_t mask_in = 1 << x;

  // these are the starting output bits
  uint16_t mask_me = 0x100;
  uint16_t mask_him = 1;
  uint16_t row = 0;
  for (int i = 0; i < 8; i++, mask_me <<= 1, mask_him <<= 1) {
    // written this way because it should compile nicely into conditional select
    // with no actual branches.  Pull out the mask bit and spread it across the
    // virtual "row"
    row |= (me[i] & mask_in) ? mask_me : 0;
    row |= (him[i] & mask_in) ? mask_him : 0;
  }

  // normalize to x, y OFF
  return row & ~(0x100 << y);
}

static void putvert(uint8_t *me, uint8_t *him, int x, uint16_t row) {
  // this time we will write out the x column so that it matches the
  // bits the row, reversing what getvert does.
  uint8_t mask_out = 1 << x;
  uint8_t hi = (uint8_t)(row >> 8);
  uint8_t mask_in = 1;
  for (int i = 0; i < 8; i++, mask_in <<= 1) {
    // either "or" in the bit, or else "~and" it out
    if (hi & mask_in)
      me[i] |= mask_out;
    else
      me[i] &= ~mask_out;

    if (row & mask_in)
      him[i] |= mask_out;
    else
      him[i] &= ~mask_out;
  }
}

// get the first diagonal, this is where y goes up when x goes up
static uint16_t getdiag1(uint8_t *me, uint8_t *him, int x, int y) {
  int d = y - x;
  uint16_t row = 0;

  uint8_t mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    int y_diag = i + d;

    // We only pull in the fragment of the row from the diagonal that makes
    // sense. We have to do this because of course all the diagonals are shorter
    // except 1. Note that extra blanks at the end of the row cannot create new
    // legal flips so skipping those bits always works
    if (y_diag < 0 || y_diag > 7)
      continue;

    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= ((me[y_diag] & mask) << 8) | (him[y_diag] & mask);
  }

  // normalize to x, y OFF
  return row & ~(0x100 << x);
}

// write back the first diagonal, this is where y goes up when x goes up
static void putdiag1(uint8_t *me, uint8_t *him, int x, int y, uint16_t row) {
  int d = y - x;
  uint8_t hi = (uint8_t)(row >> 8);

  uint8_t mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    // ask before consider just the right slice of the diagonal
    int y_diag = i + d;
    if (y_diag < 0 || y_diag > 7)
      continue;

    // either "or" in the bit, or else "~and" it out
    if (hi & mask)
      me[y_diag] |= mask;
    else
      me[y_diag] &= ~mask;

    if (row & mask)
      him[y_diag] |= mask;
    else
      him[y_diag] &= ~mask;
  }
}

// get the second diagonal, this is where y goes down when x goes up
static uint16_t getdiag2(uint8_t *me, uint8_t *him, int x, int y) {
  int d = y + x;

  uint16_t row = 0;
  int mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    int y_diag = d - i;

    // We only pull in the fragment of the row from the diagonal that makes
    // sense. We have to do this because of course all the diagonals are shorter
    // except 1. Note that extra blanks at the end of the row cannot create new
    // legal flips so skipping those bits always works
    if (y_diag < 0 || y_diag > 7)
      continue;

    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= ((me[y_diag] & mask) << 8) | (him[y_diag] & mask);
  }

  // normalize to x, y OFF
  return row & ~(0x100 << x);
}

// write back the second diagonal, this is where y goes down when x goes up
static void putdiag2(uint8_t *me, uint8_t *him, int x, int y, uint16_t row) {
  int d = y + x;
  uint8_t hi = (uint8_t)(row >> 8);

  uint8_t mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    // ask before consider just the right slice of the diagonal
    int y_diag = d - i;
    if (y_diag < 0 || y_diag > 7)
      continue;

    // either "or" in the bit, or else "~and" it out
    if (hi & mask)
      me[y_diag] |= mask;
    else
      me[y_diag] &= ~mask;

    if (row & mask)
      him[y_diag] |= mask;
    else
      him[y_diag] &= ~mask;
  }
}
