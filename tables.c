#include "board.h"

unsigned short edge[65536];
unsigned short flip_table[65536][8];

static void build_lookups(void);
static int edge_recursive(unsigned index);

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
    unsigned t = row;
    t = flip_edge_one_way(t, BLACK, i, 1);  // flip right
    t = flip_edge_one_way(t, BLACK, i, -1); // flip left

    // record the result of flipping
    // the flip table is normalized for black to move but
    // remember this is all me/him so in context the bits could
    // be black or white
    flip_table[row][i] = t;

    // now score the other outcome, WHITE gets the cell
    unsigned t2 = row;
    t2 = flip_edge_one_way(t2, WHITE, i, 1);
    t2 = flip_edge_one_way(t2, WHITE, i, -1);

    // now add the scores of the two possible outcomes to the total
    sum += edge_recursive(t) + edge_recursive(t2);
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
      // get no weight because there is a seperate edge table
      // computation that determines the value of those cells.

      if (j > 1 && j < 6)
        weighted_row_value[i] += !!(i & (1 << j));

      if (j == 1 || j == 6)
        weighted_row_value[i] -= !!(i & (1 << j));
    }
  }
}
