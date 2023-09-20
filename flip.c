#include "board.h"

extern unsigned short *(flipt[8]);
extern unsigned short *packt;

flip(board,colour,x,y)
BOARD board;
int colour;
{
  unsigned char *me,*him;
  unsigned short row, new;

  me = &board[colour][0];
  him = &board[!colour][0];

  row = gethorz(me,him,y) & (~(256<<x));
  new = flipt[x][packt[row]];
  puthorz(me,him,y,new);

  row = getvert(me,him,x,y);
  new = flipt[y][packt[row]];
  row |= (256<<y);
  if (new != row) putvert(me,him,x,new);

  row = getdiag1(me,him,x,y);
  new = flipt[x][packt[row]];
  row |= (256<<x);
  if (new != row) putdiag1(me,him,x,y,new);

  row = getdiag2(me,him,x,y);
  new = flipt[x][packt[row]];
  row |= (256<<x);
  if (new != row) putdiag2(me,him,x,y,new);
}

gethorz(me,him,y)
register unsigned char *me,*him;
{
  return ((me[y]<<8) | (him[y]));
}

puthorz(me,him,y,row)
register unsigned char *me,*him;
{
  me[y] =  (row>>8);
  him[y] = (row&0xff);
}

getvert(me,him,x,y)
register unsigned char *me,*him;
register x,y;
{
  register int row,i;

  row = 0;
  for (i=0;i<8;i++)
    row |= ((!!(me[i]&(1<<x)))<<(i+8)) |
      ((!!(him[i]&(1<<x)))<<i);
  return(row&~(1<<(y+8)));
}

putvert(me,him,x,row)
register unsigned char *me,*him;
{
  register int i,b,hi,m,mx;

  hi = row>>8;

  m=1;
  mx = (1<<x);
  for (i=0;i<8;i++,m<<=1) {
    b = me[i];
    if (hi&m) b|=mx; else b&=~mx;
    me[i] = b;

    b = him[i];
    if (row&m) b|=mx; else b&=~mx;
    him[i] = b;
  }
}

getdiag1(me,him,x,y)
register unsigned char *me,*him;
{
  register int i,d,row;

  d = y-x;

  row = 0;
  for (i=0;i<8;i++) {
    if ( (i+d)<0 || (i+d)>7 ) continue;

    row |= (!!(me[i+d]&(1<<i)))<<(i+8) |
      (!!(him[i+d]&(1<<i)))<<i;
  }
  return(row&(~(1<<(x+8))));
}

putdiag1(me,him,x,y,row)
register unsigned char *me,*him;
register row;
{
  register int hi,i,d,b,m;

  d = y-x;
  hi= row>>8;

  m = 1;
  for (i=0;i<8;i++,m<<=1) {
    if ( (i+d)<0 || (i+d)>7 ) continue;

    b = me[i+d];
    if (hi&m)  b |= m; else b &= ~m;
    me[i+d] = b;

    b = him[i+d];
    if (row&m) b |= m; else b &= ~m;
    him[i+d] = b;
  }
}

getdiag2(me,him,x,y)
register unsigned char *me,*him;
{
  register int i,d,row;

  d = y+x;

  row = 0;
  for (i=0;i<8;i++) {
    if ( (d-i)<0 || (d-i)>7 ) continue;

    row |= (!!(me[d-i]&(1<<i)))<<(i+8) |
      (!!(him[d-i]&(1<<i)))<<i;
  }
  return(row&(~(1<<(x+8))));
}

putdiag2(me,him,x,y,row)
register unsigned char *me,*him;
register row;
{
  register int hi,i,d,b,m;

  d  = y+x;
  hi = row>>8;
  m  = 1;
  for (i=0;i<8;i++,m<<=1) {
    if ( (d-i)<0 || (d-i)>7 ) continue;

    b = me[d-i];
    if (hi&m) b|=m; else b&=~m;
    me[d-i] = b;

    b = him[d-i];
    if (row&m) b|=m; else b&=~m;
    him[d-i] = b;
  }
}
