#include "board.h"

unsigned short edge[65536];
unsigned short flipt[6561][8];

static void build_lookups(void);
static void build_pack_table();
static int edge_recursive(unsigned index);

void build_tables()
{
  printf("Building general lookup tables\n");
  build_lookups();

  printf("Building pack_board_row table\n");
  build_pack_table();


  printf("Building edge table\n");
  for (int i = 0; i < 65536; i++) {
    // only process valid row combos -- no cells that are both black and white
    if (!(i & (i >> 8))) {
      edge_recursive(i);
    }
  }

  printf("Computation complete\n");
}

static int edge_recursive(unsigned row)
{
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
  if (both  == 0xff) {
    // the hi bits are "me" the low bits are "him"
    // we always evaluate like this it's not really the color it's always me/him
    edge[row] = bit_count[hi] << 9;

    // these are all no-op cases in the flip table -- attempting to flip does nothing
    // no matter where you try to flip nothing happens.
    for (int i = 0; i < 8; i++) {
      // all 8 possible moves are no-op, row already full
      flipt[pack_table[row]][i] = row;
    }

    return edge[row];
  }

  // to get the value of this edge combo, we're going to compute all the possible ways
  // it could flip to full from this position and average the final value of each
  // way.  To do this we count the number of possible flips and score it flipped each
  // of the two ways.  This isn't perfect but it is heuristically pretty good.

  // Note that we are also computing the flip table as we do this.  So we don't
  // use the table but we do populate it.

  int sum = 0;
  int count = 0;

  // normalize the row to its row number 0-6561
  int row_index = pack_table[row];

  // try flipping every bit
  for (int i = 0; i < 8; i++) {

    // if this bit is already set we skip it
    if (both & (1 << i)) {
      // but first we make this another no-op flip.
      flipt[row_index][i] = row;
      continue;
    }

    // we will flip the this cell to black and white
    count += 2;

    // ok we make a copy of the row and use our flip edge helper
    unsigned t = row;
    t = fe(t, BLACK, i, 1);  // flip right
    t = fe(t, BLACK, i, -1); // flip left

    // record the result of flipping
    // the flip table is normalized for black to move but
    // remember this is all me/him so in context the bits could
    // be black or white
    flipt[row_index][i] = t;

    // now score the other outcome, WHITE gets the cell
    unsigned t2 = row;
    t2 = fe(t2, WHITE, i, 1);
    t2 = fe(t2, WHITE, i, -1);

    // now add the scores of the two possible outcomes to the total
    sum += edge_recursive(t) + edge_recursive(t2);
  }

  // the score is the average outcome
  edge[row] = sum / count;
  return edge[row];
}

static void build_lookups()
{
  for (int i = 0; i < 256; i++) {
    // we do it in this order so that bit values has the lowest bits in the LSB
    for (int j = 7; j >= 0; j--) {
      // this is a straight bit count
      if (i & (1<<j)) {
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

// The black and white occupied slots of a board row
// are represented by two bytes of a short. But there
// are only 3 combinations, none, black, and white.
// This means you can represent a given board row as
// a base three number.  pack_table[i] is the pre-computed
// number representing the row as computed by pack_board_row.
// So literally pack_table[mask] == pack_board_row(mask)
// for all valid masks.  We skip any that have both black
// and white set.
unsigned short pack_table[65536];

static int pack_board_row(int board_row)
{
  int s = 0;

  for (int mask = 0x01; mask <= 0x80; mask <<= 1) {
    // s += (s <<1 ) multiplies s by 3 by adding it to twice itself
    // then either add 1 or 2 depending on which bit is set
    s += (s << 1) + (!!(board_row & mask)) + (((!!(board_row & (mask << 8)))) << 1);
  }

  return s;
}

// This is only used for testing... see the test code below
static int unpack_board_row(int packed_value)
{
  int d;
  int board_row = 0;

  for (int mask = 0x80; mask; mask >>= 1) {
    // pull out the first base 3 digit
    // note that the board high bits are stored in the lowest digit
    d = packed_value % 3;

    // set up to pull out the next digit
    packed_value /= 3;

    // if the digit is zero set nothing
    if (d) {
      if (d == 1) {
        // if the digit is 1 set the low bits
        board_row |= mask;
      }
      else {
        // if the digit is 2 set the high bits
        board_row |= (mask << 8);
      }
    }
  }

  return board_row;
}

// The loose row representation is 8 bits for black 8 bits for white
// but because any given cell can only have 3 actual states, black-white-empty
// the total number of valid rows is only 3^8 which is 6561.  This is much smaller
// than 2^16 (65536) almost exactly 10% the size.  So for the edge tables
// we only want to store data for valid row combos.  Hence we need a function
// that maps from a loose row to its row number.  We can do this with a lookup
// table. This code creates that table.  The code is sufficiently fast now
// that the test code is in there full time which would once have been unthinkable.
//
// A reverse mapping would be useful if we wanted to visit all row combinations
// but that bsaically doesn't happen.  So unpack only exists for this test code.
// You could quickly visit all 6561 rows and get their loose mappings saving you
// lots of loop iterations. But at this point we don't have that mapping anyway.
static void build_pack_table()
{
  // we loop through all 64k combos but we're going to prune away the invalid ones
  for (int row = 0; row < 65536; row++) {
    // any vaue where any high byte has a common bit with a low byte can be
    // skipped. It isn't a valid board row -- that would be a a row with cells that
    // are both black and white.
    if (row & (row >> 8)) continue;

    int row_index = pack_board_row(row);

    // do the reverse mapping
    int row_copy = unpack_board_row(row_index);

    // test packing/unpacking -- paranoid testing
    if (row != row_copy) {
      printf("Yipe! ");
      printf("%04x %d %04x ", row, row_index, row_copy);
      display_one_row(row);
      printf(" != ");
      display_one_row(row_copy);
      printf("\n");
      fflush(stdout);
      exit(99);
    }

    pack_table[row] = row_index;

    // This could easily be done if it were ever needed
    // extern unsigned short unpack_table[6561];
    // unpack_table[row_index] = row;
  }
}

