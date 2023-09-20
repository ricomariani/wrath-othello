#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include "endgame.h"

extern char val[256],val2[256];
extern turn;
extern short *edge;

score(board,colour)
BOARD board;
{
  register int i,s;
  register unsigned int t;
  register unsigned char *me,*him;

  me = &board[colour][0];
  him = &board[!colour][0];

  t = s = 0;
  if (turn>ENDGAME) {
    for (i=0;i<8;i++)
       s += val[me[i]];
    return(s);
  }

  for (i=2;i<6;i++) s += val2[me[i]];
  s -= val[me[1]&0x7e] + val[me[6]&0x7e] +
      ((val[me[1]&0x42] + val[me[6]&0x42])<<2);

  for (i=0;i<8;i++)
    t = (t<<1) | (!!(me[i]&(1<<7)));
  for (i=0;i<8;i++)
    t = (t<<1) | (!!(him[i]&(1<<7)));

  s += edge[t];

  t=0;
  for (i=0;i<8;i++)
    t = (t<<1) | (me[i]&1);
  for (i=0;i<8;i++)
    t = (t<<1) | (him[i]&1);

  s += edge[t] + edge[(me[0]<<8)|him[0]] + edge[(me[7]<<8)|him[7]];

  return(s);
}
