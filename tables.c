#include "board.h"

unsigned short edge[65536];
unsigned short flip_table[65536][8];

static void build_lookups(void);
static int edge_recursive(unsigned index);
static uint16_t flip_edge_one_way(uint16_t mask, int is_white, int x, int dx);

void build_tables() {
  printf("Building general lookup tables\n");
  build_lookups();

  printf("Building edge table\n");
  for (int i = 0; i < 65536; i++) {
    // only process valid row combos -- no cells that are both black and white
    if (!(i & (i >> 8))) {
      edge_recursive(i);
    }
  }

  printf("Computation complete\n");
}

static int edge_recursive(unsigned row) {
  if (edge[row]) {
    // already computed
    return edge[row];
  }

  // row is the usual loose row form with hi bits (me) and lo bits (him)
  int hi = row >> 8;
  int lo = row & 0xff;

  // these we pre-filtered
  assert((hi & lo) == 0);

  int both = (lo | hi);
  // if the row is full just count the bits
  if (both == 0xff) {
    // the hi bits are "me" the low bits are "him"
    // we always evaluate like this it's not really the color it's always me/him
    edge[row] = bit_count[hi] << 9;

    // these are all no-op cases in the flip table -- attempting to flip does
    // nothing no matter where you try to flip nothing happens.
    for (int i = 0; i < 8; i++) {
      // all 8 possible moves are no-op, row already full
      flip_table[row][i] = row;
    }

    return edge[row];
  }

  // to get the value of this edge combo, we're going to compute all the
  // possible ways it could flip to full from this position and average the
  // final value of each way.  To do this we count the number of possible flips
  // and score it flipped each of the two ways.  This isn't perfect but it is
  // heuristically pretty good.

  // Note that we are also computing the flip table as we do this.  So we don't
  // use the table but we do populate it.

  int sum = 0;
  int count = 0;

  // try flipping every bit
  for (int i = 0; i < 8; i++) {

    // if this bit is already set we skip it
    if (both & (1 << i)) {
      // but first we make this another no-op flip.
      flip_table[row][i] = row;
      continue;
    }

    // we will flip the this cell to black and white
    count += 2;

    // ok we make a copy of the row and use our flip edge helper
    unsigned tmp = row;
    tmp = flip_edge_one_way(tmp, BLACK, i, 1);  // flip right
    tmp = flip_edge_one_way(tmp, BLACK, i, -1); // flip left

    // record the result of flipping
    // the flip table is normalized for black to move but
    // remember this is all me/him so in context the bits could
    // be black or white
    flip_table[row][i] = tmp;

    // now score the other outcome, WHITE gets the cell
    unsigned t2 = row;
    t2 = flip_edge_one_way(t2, WHITE, i, 1);
    t2 = flip_edge_one_way(t2, WHITE, i, -1);

    // now add the scores of the two possible outcomes to the total
    sum += edge_recursive(tmp) + edge_recursive(t2);
  }

  // the score is the average outcome
  edge[row] = sum / count;
  return edge[row];
}

static void build_lookups() {
  for (int i = 0; i < 256; i++) {
    // we do it in this order so that bit values has the lowest bits in the LSB
    for (int j = 7; j >= 0; j--) {
      // this is a straight bit count
      if (i & (1 << j)) {
        bit_count[i]++;
        bit_values[i] <<= 4;
        bit_values[i] |= 0x8 | j;
      }

      // this doesn't count the edge slots and gives negative
      // value to the slots near the edge  so...
      //  0 -1 1 1 1 1 -1 0
      // the value of the row is the sum of the bit weights
      // for each bit that is set in that row.  So the
      // items near the end are not desireable.  The edges
      // get no weight because there is a separate edge table
      // computation that determines the value of those cells.

      if (j > 1 && j < 6)
        weighted_row_value[i] += !!(i & (1 << j));

      if (j == 1 || j == 6)
        weighted_row_value[i] -= !!(i & (1 << j));
    }
  }
}

// This helper is used to compute the overall flip table.  This is
// just doing flips along one row in one direction.  When we do the
// full flip we will use the table created by this on flipping row
// directly but we will also convert the vertical and diagonal slices
// into a virtual "row" and flip them too.  So this ends up driving
// all the flips.  We do this so that we don't have to do the expensive
// nested loop business for each row/column/diagonal when we really flip.
static uint16_t flip_edge_one_way(uint16_t row, int is_white, int x, int dx) {

  uint8_t him = (uint8_t)row;
  uint8_t me = (uint8_t)(row >> 8);
  int x0 = x;

  // normalize black/white to me/him
  if (is_white) {
    uint16_t tmp = him;
    him = me;
    me = tmp;
  }

  // note that we do not check if the current square is unoccupied
  // this is because there are many flip directions and any one of them
  // may have already filled in the current square.

  // if the adjacent square isn't an enemy square no flip is possible
  // in this direction, early out.
  if (!(him >> (x + dx) & 1)) {
    goto done;
  }

  // loop across the board advancing x by dx, the dx exit is the one
  // we will always take.
  for (;;) {
    x += dx;
    if (x < 0 || x > 7) {
      break;
    }

    // if we find our own piece there is a flip here.
    if (1 & (me >> x)) {

      // to execute the flip we go backwards until the
      // starting square, we remove the enemies pieces
      // and replace them with our own pieces
      x -= dx;
      while (x != x0) {
        me |= (1 << x);   // turn on me
        him &= ~(1 << x); // turn off him
        x -= dx;
      }

      // we're done
      break;
    }

    // we can keep trying to find a flip as long as we see enemies
    if (!(1 & (him >> x))) {
      break;
    }
  }

done:
  // In any case, we fill the square with our piece.  Note that we
  // do this even though there wasn't a flip maybe because the
  // we presume the move was valid for other reasons (maybe a flip
  // in the other direction).  This is only half the flip.
  me |= (1 << x0);

  // swap these back for black/white
  if (is_white) {
    uint16_t tmp = him;
    him = me;
    me = tmp;
  }
  return ((me << 8) | him);
}
