#include "board.h"

unsigned short edge[65536];
unsigned short flipt[6561][8];

void buildedge()
{
  printf("Building edge table\n");
  fflush(stdout);

  for (int i = 0; i < 65536; i++)
    if (!(i & (i >> 8)))
      be(i);

  printf("Computation complete\n");
  fflush(stdout);
}

int be(unsigned index)
{
  int lo, hi;
  int i, c, s;
  unsigned b, t, t2;

  if (edge[index])
    return (edge[index]);

  hi = index >> 8;
  lo = index & 0xff;

  if (hi & lo)
    return (0);

  if (lo == ((~hi) & 0xff)) {
    edge[index] = bit_count[hi] << 9;

    for (i = 0; i < 8; i++)
      flipt[pack_table[index]][i] = index;

    return (edge[index]);
  }

  b = (lo | hi);

  c = s = 0;
  for (i = 0; i < 8; i++) {
    if (b & (1 << i)) {
      flipt[pack_table[index]][i] = index;
      continue;
    }
    c += 2;

    t = index;
    t = fe(t, 0, i, 1);
    t = fe(t, 0, i, -1);

    flipt[pack_table[index]][i] = t;

    t2 = index;
    t2 = fe(t2, 1, i, 1);
    t2 = fe(t2, 1, i, -1);

    s += be(t);
    s += be(t2);
  }

  edge[index] = s / c;
  return (edge[index]);
}
