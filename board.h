#define BLACK 0
#define WHITE 1

#include <assert.h>
#include <ctype.h>
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

typedef uint8_t BOARD[2][8];

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
extern char bit_count[256];
extern uint64_t bit_values[256];
extern char weighted_row_value[256];
extern char bit_count[256];
extern char weighted_row_value[256];
extern int is_white_turn;
extern int consecutive_passes;
extern int turn;
extern unsigned short edge[65536];
extern unsigned short flip_table[65536][8];

void display(BOARD board);
void display_one_row(int row);
void display_score(BOARD board);

char BoardCharAt(BOARD board, int x, int y);
char RowCharAt(int row, int x);

void reset_scored_moves(int lvl);
void insert_scored_move(int x, int y, int score, int lvl);
int remove_scored_move(int *x, int *y, int lvl);

int valid(BOARD board, int is_white, int stack);
void flip(BOARD board, int is_white, int x, int y);
void move(BOARD board, int is_white, int x, int y);

void safe_gets(char *buf, int len);
int save();

void reset_move_stack(int lvl);
void push_move(int x, int y, int lvl);
int pop_move(int *x, int *y, int lvl);

uint16_t flip_edge_one_way(uint16_t mask, int is_white, int x, int dx);

void computer_input(BOARD board, int is_white);
int user_input(BOARD board, int is_white);

int search(BOARD board, int is_white, int *bestx, int *besty);
void move(BOARD board, int is_white, int x, int y);

void display(BOARD board);
void display_one_row(int rowbits);
void display_score(BOARD board);

void build_tables(void);

void load(const char *name, BOARD board);
int score(BOARD board, int is_white);
