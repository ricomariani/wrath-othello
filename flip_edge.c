#include "board.h"

// This helper is used to compute the overall flip table.  This is
// just doing flips along one row in one direction.  When we do the
// full flip we will use the table created by this on flipping row
// directly but we will also convert the vertical and diagonal slices
// into a virtual "row" and flip them too.  So this ends up driving
// all the flips.  We do this so that we don't have to do the expensive
// nested loop business for each row/column/diagonal when we really flip.
uint16_t flip_edge_one_way(uint16_t row, int is_white, int x, int dx) {

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
        me |= (1 << x);    // turn on me
        him &= ~(1 << x);  // turn off him
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
