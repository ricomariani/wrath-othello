#include "board.h"

int score(uint8_t *me, uint8_t *him) {
  int i;

  uint16_t tmp = 0;
  int s = 0;
  if (turn > ENDGAME) {
    // endgame scoring is just the count of bits I get
    // note that this means we don't optimize for the lowest
    // possible enemy score which means we might not play
    // truly perfectly. The wiggle room is that we might
    // be able to force more empty squares with our score
    // fixed.  This actually comes up in the game in endgame.txt
    for (i = 0; i < 8; i++) {
      s += bit_count[me[i]];
    }
    return s;
  }

  // use the weighted value for the rows that are in "the middle"
  for (i = 2; i < 6; i++) {
    s += weighted_row_value[me[i]];
  }

  // Note that row 1 and 6 are never counted, they get zero score,
  // except for the penalty squares below which get negative score.

  // the square one position away from the corners are considered
  // very bad indeed
  s -= bit_count[me[1] & 0x7e] + bit_count[me[6] & 0x7e] +
       ((bit_count[me[1] & 0x42] + bit_count[me[6] & 0x42]) << 2);

  // make a virtual row that consists of the last column
  for (i = 0; i < 8; i++) {
    tmp = (tmp << 1) | (!!(me[i] & (1 << 7)));
  }

  for (i = 0; i < 8; i++) {
    tmp = (tmp << 1) | (!!(him[i] & (1 << 7)));
  }

  // add the edge score
  s += edge[tmp];

  // make a virtual row that consists of the first column
  tmp = 0;
  for (i = 0; i < 8; i++) {
    tmp = (tmp << 1) | (me[i] & 1);
  }

  for (i = 0; i < 8; i++) {
    tmp = (tmp << 1) | (him[i] & 1);
  }

  // add the edge scores of the column plus first and last row
  s += edge[tmp] + edge[(me[0] << 8) | him[0]] + edge[(me[7] << 8) | him[7]];

  return (s);
}
