#include "board.h"

extern unsigned short *(flipt[8]);
extern unsigned short *packt;

#define stuff(a,b,c) (((((unsigned long)a)&0xff)<<16)|((((unsigned long)b)&0xff)<<8)|((c)&0xff))
#define unstuff(a) ((((a)>>16)|((a)>>8)|(a))&0xff)

valid(board,colour,stack)
BOARD board;
{
	unsigned char *me,*him;
	unsigned long y0,y1,d0,d1;
	int i,y;
	unsigned char used;
	int yes;
	unsigned row;

	new(stack);
	yes = 0;

	me = &board[colour][0];
	him = &board[!colour][0];

	for (y=0;y<8;y++) {
		row = (me[y]<<8)|him[y];
		used =  d0  = (row|(row>>8));
		d1  = 1;
		for (i=0;i<8;i++,d1<<=1) {
			if (d0&d1) continue;
			if ( (row|(256<<i)) != flipt[i][packt[row]]) {
				push(i,y,stack);
				used |= (1<<i);
				yes++;
			}
		}

		if (used == 0xff) continue;

		y1 = stuff(used,used,used);
		y0 = 0;
		for (i=y-1;i>=0;i--) {
			d0 = stuff(him[i]>>(y-i),him[i]<<(y-i),him[i]);
			d1 = stuff(me[i]>>(y-i),me[i]<<(y-i),me[i]);
			y0 = ((~y1)&d0)|(y0&(y1|d1));
			y1 |= d1|(~d0);
			if ((~y1) == 0) break;
		}
		y0 &= y1;
		row = unstuff(y0);

		y1 = stuff(used|row,used|row,used|row);
		y0 = 0;
		for (i=y+1;i<8;i++) {
			d0 = stuff(him[i]>>(i-y),him[i]<<(i-y),him[i]);
			d1 = stuff(me[i]>>(i-y),me[i]<<(i-y),me[i]);
			y0 = ((~y1)&d0)|(y0&(y1|d1));
			y1 |= d1|(~d0);
			if ((~y1) == 0) break;
		}

		y0  &= y1;
		row |= unstuff(y0);
		row &= ~(used);
		used |= row;
		if (row) {
			for (i=0;i<8;i++) {
				if (row & (1<<i)) {
					push(i,y,stack);
					yes++;
				}
			}
		}
	}
	return(yes);
}
