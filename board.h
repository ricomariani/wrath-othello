#define BLACK 0
#define WHITE 1

typedef unsigned char BOARD[2][8];
typedef unsigned char *LAYER;

extern BOARD initial;
extern char ascii_values[];
extern char bit_count[256];
extern char weighted_row_value[256];
extern char bit_count[256];
extern char weighted_row_value[256];
extern int colour;
extern int consecutive_passes;
extern int turn;
extern unsigned short *edge;
extern unsigned short *(flipt[8]);
extern unsigned short *pack_table;

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
unsigned short *edge;
unsigned short *(flipt[8]);

void buildedge(void);
int be(unsigned index);
void build_pack_table(void);

void load(const char *name, BOARD board);
int score(BOARD board, int colour);
