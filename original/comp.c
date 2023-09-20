#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "endgame.h"

extern char val[256];
extern char val2[256];
extern int turn;
extern int pass;

computer_input(board,colour)
BOARD board;
{
	int x,y,sc;

	sc = search(board,colour,&x,&y);
	if (x==-1) {
		printf("%c has to pass.\n",colour?'W':'B');
		fflush(stdout);
		pass++;
	}
	else {
		printf("best move for %c is at %c%c (score %d)\n",
			colour?'W':'B',x+'a','8'-y, sc - ((turn>ENDGAME)?0:(8187)));
		fflush(stdout);
		move(board,colour,x,y);
		pass=0;
	}
}
