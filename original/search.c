#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include <setjmp.h>
#include "endgame.h"
#include <signal.h>
#include <string.h>

#define HORRIBLE -32000
#define GREAT     32000

extern int turn;
long int boards;
jmp_buf env;
int totaltime;
int starttime;
int thistime;
static pass;
int bx,by;
int bs;

static limit;
int IRQ;


void timeout()
{
  IRQ = 1;
}

search(board,colour,bestx,besty)
BOARD board;
int *bestx, *besty;
{
	int i,start,lvl,moves;

  signal(SIGALRM,timeout);

	if (turn < 15)
		limit = 2;
	else if (turn < 30)
		limit = 4;
	else 
		limit = 10;

	boards = 0;
	pass = 0;
  IRQ = 0;

	starttime=time(0);
	if (turn <= ENDGAME) {
    alarm(limit);
		start = 2 ;
  }
	else {
		printf("Seeking to end of board\n");
		fflush(stdout);
		start = 64-turn;
		if (start%2) start++;
	}

	if (setjmp(env)) {
		*bestx = bx;
		*besty = by;
		thistime = time(0)-starttime;
		if (!thistime) thistime =1;
		totaltime += thistime;
		printf("\nEvaluated %ld boards in %d:%02d (%ld boards/sec). ",
				boards,thistime/60,thistime%60,boards/thistime);
		printf("Total time used=%d:%02d \n", totaltime/60,totaltime%60);
		fflush(stdout);
		return(bs);
	}

	if (!(moves = valid(board,colour,start))) {
		bx = -1;
		by = -1;
		bs = HORRIBLE;
		goto no_moves;
	}

	newlist(0);
	while (pop(bestx,besty,start)) {
		insert(*bestx,*besty,0,0);
	}

	lvl = 0;
	for (i=start;i<65;i+=2) {
		printf("%2d: ",i);
		fflush(stdout);
		rsearch(board,colour,i,lvl);
		if (i+2>64-turn) break;
		lvl = !lvl;
	}

no_moves:
  alarm(0);
	longjmp(env,1);
}


rsearch(board,colour,rec,lvl)
BOARD board;
{
	int x,y,sc;
	BOARD brd;
	int moves;

	newlist(!lvl);
	moves = 0;
	pass = 0;

	if (!recmove(&x,&y,lvl)) return 0;

	printf("%c%c=",x+'a','8'-y);
	fflush(stdout);
	bcpy(brd,board);
	flip(brd,colour,x,y);

	bs = mini(brd,!colour,rec-1,HORRIBLE,GREAT);
	bx = x;
	by = y;
	printf("%d  ",bs-((turn>ENDGAME)?0:8187));
	fflush(stdout);
	insert(x,y,bs,!lvl);

	while (recmove(&x,&y,lvl)) {
		printf("%c%c",x+'a','8'-y);
		fflush(stdout);
		bcpy(brd,board);
		flip(brd,colour,x,y);
		sc = mini(brd,!colour,rec-1,bs,GREAT);
		insert(x,y,sc,!lvl);
		if (sc > bs) {
			putchar('=');
			bx = x; by = y; bs = sc;
		}
		else 
			putchar('<');
		
		printf("%d  ",sc-((turn>ENDGAME)?0:8187));
		fflush(stdout);
	}
	printf("\n");
	fflush(stdout);
	return(bs);
}

mini(board,colour,rec,a,b)
BOARD board;
{
	if (IRQ) longjmp(env,1);
	boards++;
	if (!rec)
		return(score(board,colour));
	else {
		BOARD brd;
		int x,y,sc,yes;

		new(rec);

		yes = valid(board,colour,rec);
		if (!yes) {
			if (turn > ENDGAME && !pass) {
				pass++;
				sc = maxi(board,!colour,rec+1,a,b);
			}
			else
				sc = maxi(board,!colour,rec-1,a,b);
			return(sc);
		}
		pass = 0;

		while (pop(&x,&y,rec)) {
			bcpy(brd,board);
			flip(brd,colour,x,y);
			sc = maxi(brd,!colour,rec-1,a,b);
			if (sc < b ) {
				b = sc;
				if (b <= a) return(b);
			}
		}
		return(b);
	}
}

maxi(board,colour,rec,a,b)
BOARD board;
{
	if (IRQ) longjmp(env,1);
	boards++;
	if (!rec)
		return(score(board,colour));
	else {
		BOARD brd;
		int x,y,sc,yes;

		new(rec);

		yes = valid(board,colour,rec);
		if (!yes) {
			if (turn>ENDGAME && !pass) {
				pass++;
				sc = mini(board,!colour,rec+1,a,b);
			}
			else
				sc = mini(board,!colour,rec-1,a,b);
			return(sc);
		}
		pass = 0;

		while (pop(&x,&y,rec)) {
			bcpy(brd,board);
			flip(brd,colour,x,y);
			sc = mini(brd,!colour,rec-1,a,b);
			if (sc > a ) {
				a = sc;
				if (a >= b) return(a);
			}
		}
		return(a);
	}
}

bcpy(b1,b2)
register char *b1,*b2;
{
	register i;

	memcpy(b1,b2, sizeof(BOARD));

	// for (i=0;i<sizeof(BOARD);i++) *b1++ = *b2++;
}

move(board,colour,x,y)
register BOARD board;
{
	board[colour][y] |= (1<<x);
	flip(board,colour,x,y);
	display(board);
}

show(rec,score)
{
	int i;
	for (i=0;i<rec;i++)
		putchar('\t');
	printf("%d\n",score);
	fflush(stdout);
}
