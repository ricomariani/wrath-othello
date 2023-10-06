#define BLACK 0
#define WHITE 1

#include <inttypes.h>
#include <memory.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

typedef unsigned char BOARD[2][8];
typedef unsigned char *LAYER;

// At move 44 (below) we will seek to the end of the game (20 ply)
// This takes about 90 seconds on a modern processor.  It used to take
// about 10 minutes this number was more like 50 IIRC. From this
// point on the game plays perfectly and the only score needed
// it count of pieces.  Hueristics are out because we can see
// all the way to the end. The faster scoring helps this to finish
// sooner.
#define ENDGAME 44

// By happenstance it works out that a neutral board score is 8187, 
// all the scores are positive, so we subtract that out to make it
// seem positive or negative for bad/good.
#define SCORE_BIAS 8187

extern BOARD initial;
extern char ascii_values[];
extern char bit_count[256];
extern uint64_t bit_values[256];
extern char weighted_row_value[256];
extern char bit_count[256];
extern char weighted_row_value[256];
extern int colour;
extern int consecutive_passes;
extern int turn;
extern unsigned short edge[65536];
extern unsigned short flipt[65536][8];

void display(BOARD board);
void display_one_row(int row);
void display_score(BOARD board);

void reset_scored_moves(int lvl);
void insert_scored_move(int x, int y, int score, int lvl);
int remove_scored_move(int *x, int *y, int lvl);

int valid(BOARD board, int colour, int stack);
void flip(BOARD board, int colour, int x, int y);
void move(BOARD board, int colour, int x, int y);

void safe_gets(char *buf, int len);
int save();

void reset_move_stack(int lvl);
void push(int x, int y, int lvl);
int pop_move(int *x, int *y, int lvl);

unsigned fe(unsigned mask, int colour, int x, int dx);

void computer_input(BOARD board, int colour);
int user_input(BOARD board, int colour);

int search(BOARD board, int colour, int *bestx, int *besty);
void move(BOARD board, int colour, int x, int y);

void display(BOARD board);
void display_one_row(int rowbits);
void display_score(BOARD board);

void build_tables(void);

void load(const char *name, BOARD board);
int score(BOARD board, int colour);
