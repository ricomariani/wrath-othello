#define BLACK 0
#define WHITE 1

typedef unsigned char BOARD[2][8];
typedef unsigned char *LAYER;

extern char val[256];
extern char val2[256];
extern int turn;
extern int consecutive_passes;

void display(BOARD board);
void display_one_row(int row);
void display_score(BOARD board);

void reset_scored_moves(int lvl);
void insert_scored_move(int x, int y, int score, int lvl);
int remove_scored_move(int *x, int *y, int lvl);

int valid(BOARD board, int colour, int stack);
void flip(BOARD board, int colour, int x, int y);
