#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

unsigned short *edge;
unsigned short *(flipt[8]);
extern unsigned char val[256];
extern unsigned short *pack_table;

void buildedge()
{
  unsigned long i;

  for (i = 0; i < 8; i++) {
    flipt[i] = (unsigned short *)malloc(6561 * sizeof(short));
    if (!flipt[i]) {
      printf("Error, can't allocate enough memory for flip table\n");
      fflush(stdout);
      exit(1000);
    }
  }

  edge = (unsigned short *)calloc(65536, sizeof(short));
  if (!edge) {
    printf("Error, can't allocate enough memory for edge table\n");
    fflush(stdout);
    exit(1);
  }

  printf("Building edge table\n");
  fflush(stdout);

  for (i = 0; i < 65536; i++)
    edge[i] = 0;

  for (i = 0; i < 65536; i++)
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
    edge[index] = val[hi] << 9;

    for (i = 0; i < 8; i++)
      flipt[i][pack_table[index]] = index;

    return (edge[index]);
  }

  b = (lo | hi);

  c = s = 0;
  for (i = 0; i < 8; i++) {
    if (b & (1 << i)) {
      flipt[i][pack_table[index]] = index;
      continue;
    }
    c += 2;

    t = index;
    t = fe(t, 0, i, 1);
    t = fe(t, 0, i, -1);

    flipt[i][pack_table[index]] = t;

    t2 = index;
    t2 = fe(t2, 1, i, 1);
    t2 = fe(t2, 1, i, -1);

    s += be(t);
    s += be(t2);
  }

  edge[index] = s / c;
  return (edge[index]);
}
