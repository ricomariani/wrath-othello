#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

unsigned short *packt;

bpack()
{
  register long i;

  packt = (unsigned short *)calloc(65536,sizeof(short));

  if (!packt) {
    printf("Error, can't allocate enough memory for pack table\n");
    fflush(stdout);
    exit(1);
  }
  printf("Building pack table\n");
  fflush(stdout);

  for (i=0;i<65536;i++)
    if (!(i&(i>>8))) packt[i] = pack(i);
}

pack(mask)
register mask;
{
  register s,i;
  s = 0;

  for (i=0;i<8;i++)
    s += (s<<1) + (!!(mask&(1<<i))) + ((!!(mask&(1<<(i+8))))<<1);
  return(s);
}

unpack(et)
register et;
{
  register d,mask,i;

  mask = 0;

  for (i=7;i>=0;i--) {
    d = et%3;
    et /= 3;
    if (d)
        if (d==1)
        mask |= (1<<i);
        else
        mask |= (1<<(i+8));
  }
  return(mask);
}
