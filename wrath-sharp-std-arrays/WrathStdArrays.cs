using System;
using System.Diagnostics;
using System.Numerics;
using System.Reflection;
using System.Reflection.Metadata;
using System.Runtime.Intrinsics;
using Microsoft.Win32.SafeHandles;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


class Othello {

int turn = 0;
int consecutive_passes = 0;
byte is_white_turn = 1;

readonly int ENDGAME = 44;
readonly int SCORE_BIAS = 8187;
readonly byte INITIAL_DEPTH = 0;
readonly byte BLACK = 0;
readonly byte WHITE = 1;

// this gives the value of an edge
// you hace to flip the vertical bits
static ushort[] edge = new ushort[65536];

// flipt[row][x] tell us how to flip
// the row if we place a piece on
// the given row, in column x.  You have
// to extract the columns and diagonals
// to use this elsewhere
static ushort[,] flipt = new ushort[65536,8];

// This tells us how many bits are set
// in this byte.  Useful for endgame
// scoring and current score display.
static byte[] bit_count = new byte[256];

// this gives the value of a row I own
// in the middle of the board (not the edges)
static byte[] weighted_row_value = new byte[256];

// This has the bit numbers lie 0x05 encodes to 0xc8
// one nibble encodes each bit position the high bit is set
// to make the nibble non-zero if there is a bit there
// so you can read out the bits with &7 then >>4
static ulong[] bit_values = new ulong[256];

public class TimeoutException : Exception
{
    public TimeoutException(string message) : base(message)
    {
    }
}

class BOARD {
  int n;
  public byte[] w;
  public byte[] b;

  public BOARD() {
     w = new byte[8];
     b = new byte[8];
  }

  public BOARD clone() {
    var brd = storedBoards[n+1];
    brd.n = n+1;
    Array.Copy(this.w, brd.w, 8);
    Array.Copy(this.b, brd.b, 8);
    return brd;
  }

  public byte get(byte is_white, int j) {
    return (is_white != 0 ? w : b)[j];
  }

  public void put(byte is_white, int j, byte val) {
    (is_white != 0 ? w : b)[j] = val;
  }

  public byte[] half(byte is_white) {
    return is_white != 0 ? w : b;
  }
}

static BOARD[] storedBoards = new BOARD[32];


// the computer provides "input" for the next move
// it's done uniformly like this so that the logic for
// player vs. computer is basically the same as computer
// vs. itself.
void computer_input(BOARD board, byte is_white)
{
  byte x, y;
  int score = search(board, is_white, out x, out y);

  if (x == 0xff) {
    Console.WriteLine("{0} has to pass.", is_white != 0 ? 'W' : 'B');
    consecutive_passes++;
  } else {
    // at the endgame the score is the number of pieces we have
    int score_bias = turn > ENDGAME ? 0 : SCORE_BIAS;
    Console.WriteLine("best move for {0} is at {1}{2} (score {3})", is_white != 0 ? 'W' : 'B',
           (char)(x + 'a'), (char)('8' - y), score - score_bias);

    move(board, is_white, x, y);
    consecutive_passes = 0;
  }
}

// here we ask the user what they want to do.
bool user_input(BOARD board, byte is_white)
{
again:;
  // user input x and y
  int user_x = -1;
  int user_y = -1;

  reset_move_stack(INITIAL_DEPTH);

  // recompute the valid moves and put them on the stack
  // they go on stack number INITIAL_DEPTH (i.e. the root)
  valid(board, is_white, INITIAL_DEPTH);

  Console.Write("Please enter a move --> ");

  string? s = Console.ReadLine();
  if (s == null) {
    // end of file
    Environment.Exit(0);
  }

  int len = s.Length;

  bool user_pass = false;

  for (int i = 0; i < len; i++) {
    if (s[i] == '?') {
      // help
      Console.WriteLine("");
      Console.WriteLine("?\t\t\t\t:display this help page");
      Console.WriteLine("[a-z][1-8]\t\t\t:enter a move");
      Console.WriteLine("p\t\t\t\t:pass this turn");
      Console.WriteLine("r\t\t\t\t:redraw the board");
      Console.WriteLine("s\t\t\t\t:save current game");
      Console.WriteLine("q\t\t\t\t:quit current game");
      Console.WriteLine("");
      goto again;
    }
    if (s[i] >= 'a' && s[i] <= 'h') {
      // column
      user_x = s[i] - 'a';
    }
    if (s[i] >= '1' && s[i] <= '8') {
      // row
      user_y = '8' - s[i];
    }
    if (s[i] == 'p') {
      // pass
      user_pass = true;
    }
    if (s[i] == 'q') {
      // quit
      Environment.Exit(0);
    }
    if (s[i] == 'r') {
      // redraw the display
      display(board);
      display_score(board);
      goto again;
    }
    if (s[i] == 's') {
      if (save()) {
        goto again;
      }
      else {
        Environment.Exit(0);
      }
    }
  }

  if ((user_x == 0xff || user_y == 0xff) && !user_pass) {
    Console.WriteLine("?syntax error\n");
    goto again;
  }

  byte x, y;

  if (user_pass) {
    // make sure there are no moves
    if (pop_move(out x, out y, INITIAL_DEPTH)) {
      Console.WriteLine("You can't pass unless you have no moves");
      goto again;
    }
    consecutive_passes++;
    Console.WriteLine("Pass accepted.\n");
    return false;
  }

  // make sure that the entered move is a valid move
  while (pop_move(out x, out y, INITIAL_DEPTH)) {
    if (x == user_x && y == user_y) {
      Console.WriteLine("Move to {0}{1} accepted.\n", (char)(x + 'a'), (char)('8' - y));
      move(board, is_white, x, y);
      consecutive_passes = 0;
      return false;
    }
  }

  Console.WriteLine("You can't move to {0}{1}.\n", (char)(user_x + 'a'), (char)('8' - user_y));
  goto again;
}


void move(BOARD board, byte is_white, int x, int y)
{
  board.put(is_white, y, (byte)(board.get(is_white, y) | (1<<x)));
  flip(board, is_white, x, y);
  display(board);
}

void show(int depth, int score)
{
  int i;

  for (i = 0; i < depth; i++)
    Console.Write('\t');

  Console.WriteLine("{0}", score);
}

string ascii_values = "-BW?";

// this gets the type of the row board[0] is black and board[1] is white
// this is used only for display
char BoardCharAt(BOARD board, int x, int y) {
  int i1 = (board.get(0, y) >> x) & 1;
  int i2 = (board.get(1, y) >> x) & 1;
  return ascii_values[i1 + (i2 << 1)];
}

// here row has a single row, black is the high bits and white is the low bits
// the values are 0 empty, 1 black and 2 white just like the ascii table above
// this is used only for display
char RowCharAt(int row, int x) {
   return ascii_values[((row >> (8 + x)) & 1) + 2 * ((row >> x) & 1)];
}

// draw all the rows of the board
void display(BOARD board)
{
  int  x, y;

  for (y = 0; y < 8; y++) {
    // row label (digit)
    Console.Write("\t\t\t\t{0} ", (char)('8' - y));

    // board data
    for (x = 0; x < 8; x++) {
      Console.Write(BoardCharAt(board, x, y));
      Console.Write(' ');
    }
    Console.Write('\n');
  }
  Console.Write("\t\t\t\t  ");
  // column label (letter)
  for (x = 0; x < 8; x++) {
    Console.Write((char)('a' + x));
    Console.Write(' ');
  }
  Console.Write('\n');
}

BOARD initial = new BOARD();

void display_one_row(int rowbits) {
  for (int x = 0; x < 8; x++) {
    Console.Write(RowCharAt(rowbits, x));
  }
}


// this is the raw score, just counts
// the evaluation is not this at all.
void display_score(BOARD board)
{
  int sc1, sc2, i;

  sc1 = 0;
  for (i = 0; i < 8; i++) {
    sc1 += bit_count[board.get(1, i)];
  }

  Console.Write("\t\t\t\t\t\t    Score: W{0} ", sc1);

  sc2 = 0;
  for (i = 0; i < 8; i++) {
    sc2 += bit_count[board.get(0, i)];
  }

  Console.Write("B={0}\n", sc2);
  turn = sc1 + sc2;
}

void build_lookups()
{
  for (int i = 0; i < 256; i++) {
    // we do it in this order so that bit values has the lowest bits in the LSB
    for (int j = 7; j >= 0; j--) {
      // this is a straight bit count
      if ((i & (1<<j)) != 0) {
        bit_count[i]++;
        bit_values[i] <<= 4;
        bit_values[i] |= 0x8 | (ulong)j;
      }

      // this doesn't count the edge slots and gives negative
      // value to the slots near the edge  so...
      //  0 -1 1 1 1 1 -1 0
      // the value of the row is the sum of the bit weights
      // for each bit that is set in that row.  So the
      // items near the end are not desireable.  The edges
      // get no weight because there is a seperate edge table
      // computation that determines the value of those cells.

      if (j > 1 && j < 6) {
        weighted_row_value[i] += (byte)((i & (1 << j)) != 0 ? 1 : 0);
      }

      if (j == 1 || j == 6) {
        weighted_row_value[i] -= (byte)((i & (1 << j)) != 0 ? 1 : 0);
      }
    }
  }
}

void build_tables()
{
  for (int i = 0; i < 32; i++) {
    stacks[i] = new Stack();
  }

  Console.WriteLine("Building general lookup tables");
  build_lookups();

  Console.WriteLine("Building edge table");

  for (int i = 0; i < 65536; i++) {
    // only process valid row combos -- no cells that are both black and white
    if ((i & (i >> 8)) == 0) {
      edge_recursive((ushort)i);
    }
  }

  Console.WriteLine("Computation complete");
}

int edge_recursive(ushort row)
{
  if (edge[row] != 0) {
    // already computed
    return edge[row];
  }

  // row is the usual loose row form with hi bits (me) and lo bits (him)
  int hi = row >> 8;
  int lo = row & 0xff;

  // these we pre-filtered
  if ((hi & lo) != 0) {
    throw new Exception("bogus recursive call");
  }

  int both = (lo | hi);
  // if the row is full just count the bits
  if (both  == 0xff) {
    // the hi bits are "me" the low bits are "him"
    // we always evaluate like this it's not really the color it's always me/him
    edge[row] = (ushort)(((int)bit_count[hi]) << 9);

    // these are all no-op cases in the flip table -- attempting to flip does nothing
    // no matter where you try to flip nothing happens.
    for (int i = 0; i < 8; i++) {
      // all 8 possible moves are no-op, row already full
      flipt[row, i] = row;
    }

    return edge[row];
  }

  // to get the value of this edge combo, we're going to compute all the possible ways
  // it could flip to full from this position and average the final value of each
  // way.  To do this we count the number of possible flips and score it flipped each
  // of the two ways.  This isn't perfect but it is heuristically pretty good.

  // Note that we are also computing the flip table as we do this.  So we don't
  // use the table but we do populate it.

  int sum = 0;
  int count = 0;

  // normalize the row to its row number 0-6561
  int row_index = row;

  // try flipping every bit
  for (int i = 0; i < 8; i++) {

    // if this bit is already set we skip it
    if ((both & (1 << i)) != 0) {
      // but first we make this another no-op flip.
      flipt[row_index, i] = row;
      continue;
    }

    // we will flip the this cell to black and white
    count += 2;

    // ok we make a copy of the row and use our flip edge helper
    ushort t = row;
    t = fe(t, BLACK, i, 1);  // flip right
    t = fe(t, BLACK, i, -1); // flip left

    // record the result of flipping
    // the flip table is normalized for black to move but
    // remember this is all me/him so in context the bits could
    // be black or white
    flipt[row_index, i] = t;

    // now score the other outcome, WHITE gets the cell
    ushort t2 = row;
    t2 = fe(t2, WHITE, i, 1);
    t2 = fe(t2, WHITE, i, -1);

    // now add the scores of the two possible outcomes to the total
    sum += edge_recursive(t) + edge_recursive(t2);
  }

  // the score is the average outcome
  edge[row] = (ushort)(sum / count);
  return edge[row];
}

ushort fe(ushort row, byte is_white, int x, int dx)
{
  int i, him, me, x0;

  him = row & 0xff;
  me = row >> 8;
  x0 = x;

  if (is_white != 0) {
    i = him;
    him = me;
    me = i;
  }

  if ((him & (1 << (x + dx))) == 0)
    goto done;

  for (i = 0; i < 8; i++) {
    x += dx;
    if (x < 0 || x > 7)
      goto done;

    if ((me & (1 << x)) != 0) {
      x -= dx;
      while (x != x0) {
        me |= (1 << x);
        him &= ~(1 << x);
        x -= dx;
      }
      goto done;
    }
    if ((him & (1 << x)) == 0)
      goto done;
  }

done:
  me |= (1 << x0);
  if (is_white != 0) {
    i = him;
    him = me;
    me = i;
  }
  return (ushort)((me << 8) | him);
}

void flip(BOARD board, byte is_white, int x, int y)
{
  byte other = (byte)(1-is_white);
  var me = board.half(is_white);
  var him = board.half(other);

  ushort row = (ushort)(gethorz(me, him, y) & (~(256 << x)));

  ushort new_row = flipt[row, x];
  puthorz(me, him, y, new_row);

  row = getvert(me, him, x, y);
  new_row = flipt[row, y];
  row |= (ushort)(256 << y);
  if (new_row != row) {
    putvert(me, him, x, new_row);
  }

  row = getdiag1(me, him, x, y);
  new_row = flipt[row, x];
  row |= (ushort)(256 << x);
  if (new_row != row)
    putdiag1(me, him, x, y, new_row);

  row = getdiag2(me, him, x, y);
  new_row = flipt[row, x];
  row |= (ushort)(256 << x);
  if (new_row != row)
    putdiag2(me, him, x, y, new_row);

}

ushort gethorz(byte[] me, byte[] him, int y)
{
  return (ushort)((me[y] << 8) | him[y]);
}

void puthorz(byte[] me, byte[] him, int y, ushort row)
{
  me[y] = (byte)(row >> 8);
  him[y] = (byte)(row & 0xff);
}

ushort getvert(byte[] me, byte[] him, int x, int y)
{
  ushort row = 0;
  ushort mask = (ushort)(1 << x);
  for (int i = 0; i < 8; i++) {
    row |= (ushort)(((( me[i] & mask)) != 0 ? 1 : 0) << (i + 8));
    row |= (ushort)((((him[i] & mask)) != 0 ? 1 : 0) << i);
  }
  row &= (ushort)~(1 << (y + 8));
  return row;
}

void putvert(byte[] me, byte[] him, int x, ushort row)
{
  byte hi = (byte)(row >> 8);
  byte mask = 1;
  byte mx = (byte)((1 << x));

  for (int i = 0; i < 8; i++, mask <<= 1) {
    byte b = me[i];
    if ((hi & mask) != 0)
      b |= mx;
    else
      b &= (byte)~mx;
    me[i] = b;

    b = him[i];
    if ((row & mask) != 0)
      b |= mx;
    else
      b &= (byte)~mx;
    him[i] = b;
  }
}

ushort getdiag1(byte[] me, byte[] him, int x, int y)
{
  int i, d;

  d = y - x;

  ushort row = 0;
  for (i = 0; i < 8; i++) {
    if (i + d < 0 || i + d > 7)
      continue;

    row |= (ushort)(((me[i + d] >> i) & 1) << (i + 8) | ((him[i + d] >> i) & 1) << i);
  }
  row &= (ushort)~(1 << (x + 8));
  return row;
}

void putdiag1(byte[] me, byte[] him, int x, int y, int row)
{
  int d = y - x;
  byte hi = (byte)(row >> 8);

  byte mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    if ((i + d) < 0 || (i + d) > 7)
      continue;

    byte b = me[i + d];
    if ((hi & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    me[i + d] = b;

    b = him[i + d];
    if ((row & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    him[i + d] = b;
  }
}

ushort getdiag2(byte[] me, byte[] him, int x, int y)
{
  int d = y + x;

  ushort row = 0;
  for (int i = 0; i < 8; i++) {
    if ((d - i) < 0 || (d - i) > 7)
      continue;

    row |= (ushort)(((me[d - i] >> i) & 1) << (i + 8) | ((him[d - i] >> i) & 1) << i);
  }
  row &= (ushort)~(1 << (x + 8));
  return row;
}

void putdiag2(byte[] me, byte[] him, int x, int y, ushort row)
{
  int d = y + x;
  byte hi = (byte)(row >> 8);
  byte mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    if ((d - i) < 0 || (d - i) > 7)
      continue;

    byte b = me[d - i];
    if ((hi & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    me[d - i] = b;

    b = him[d - i];
    if ((row & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    him[d - i] = b;
  }
}

bool test_mode = false;

static void Main(string[] args)
{
  (new Othello()).Begin(args);
}

public void Begin(string[] args)
{
  Console.WriteLine("Framework Version: {0}", Environment.Version);

  for (int i = 0; i < 32; i++) {
     storedBoards[i] = new BOARD();
     stacks[i] = new Stack();
  }

  initial.w[3] = (byte)0x10;
  initial.w[4] = (byte)0x08;
  initial.b[3] = (byte)0x08;
  initial.b[4] = (byte)0x10;

  build_tables();

  bool player = false;
  byte player_is_white = 0;

  if (args.Length > 0) {
    var arg1 = args[0];
    switch (arg1[0]) {

    case 'w':
    case 'W':
      player = true;
      Console.WriteLine("You will be playing white.");
      player_is_white = 1;
      break;

    case 'b':
    case 'B':
      player = true;
      Console.WriteLine("You will be playing black.\n");
      player_is_white = 0;
      break;
    }

    if (arg1.Length >= 2)
    switch (arg1[1]) {
    case 'l':
    case 'L':
      if (args.Length < 2) {
        Console.WriteLine("wrath: You must specify a file name");
        Environment.Exit(1);
      }
      // also sets is_white_turn
      initial = load(args[1]);

      if (arg1.Length >= 3 && (arg1[2] == 't' || arg1[2] == 'T')) {
         // do one move and stop
         test_mode = true;
      }
      break;
    }
  }

  // start with no passes
  consecutive_passes = 0;

  // display the initial board and score
  display(initial);
  display_score(initial);


  // repeat play until there are two passes, alternating color
  while (consecutive_passes < 2) {
    if (player && is_white_turn == player_is_white)
      user_input(initial, is_white_turn);
    else
      computer_input(initial, is_white_turn);

    display_score(initial);
    is_white_turn = (byte)(1 -is_white_turn);

    if (test_mode) {
      Console.WriteLine("test complete\n");
      Environment.Exit(0);
    }
  }
}

bool save()
{
  Console.Write("filename (press return to abort): ");
  string? name = Console.ReadLine();
  if (name == null) {
    return true;
  }

  var sw = new StreamWriter(name);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      sw.Write(BoardCharAt(initial, x, y));
      sw.Write(' ');
    }
    sw.Write('\n');
  }
  sw.Write('\n');
  char player = is_white_turn != 0 ? 'w' : 'b';
  sw.WriteLine("{0} to play", player);
  return false;
}

BOARD load(string name)
{
  var sr = new StreamReader(name);

  var board = new BOARD();

  for (int i = 0; i < 8; i++) {

    for (int j = 0; j < 8; ) {
      int b = sr.Read();
      if (b == -1) {
        Console.WriteLine("wrath: Unable to parse input board");
        Environment.Exit(1);
      }

      if (char.IsWhiteSpace((char)b)) continue;

      switch (b) {

      case 'B':
      case 'b':
        board.put(0, i, (byte)(board.get(0, i) | (1 << j)));
        break;

      case 'W':
      case 'w':
        board.put(1, i, (byte)(board.get(1, i) | (1 << j)));
        break;

      case '-':
        break;

      default:
        Console.WriteLine("wrath: Board has invalid characters");
        Environment.Exit(1);
        break;
      }

      j++;
    }
  }

  for (;;) {
    int b = sr.Read();
    if (b == -1) {
      Console.WriteLine("wrath: Unable to parse input board");
      Environment.Exit(1);
    }

    if (char.IsWhiteSpace((char)b)) continue;

    switch (b) {
    case 'w':
    case 'W':
      is_white_turn = 1;
      break;

    case 'b':
    case 'B':
      is_white_turn = 0;
      break;

    default:
      Console.WriteLine("wrath: I can't tell whose turn is next");
      Environment.Exit(1);
      break;
    }

    Console.WriteLine("Picking up where we left off... {0} to play",
          is_white_turn != 0 ? "White" : "Black");
    break;
  }

  return board;
}

// We keep moves we are considering here, this is for holding the next set of valid moves
class Stack {
  public byte top;
  public ushort[] xy;

  public Stack() {
    xy = new ushort[32];
  }
}

// these are all the stacks we will ever need, no malloc
Stack[] stacks = new Stack[32];

void reset_move_stack(int lvl)
{
  stacks[lvl].top = 0;
}

// each next valid move at this recursion level is pushed on its own stack
void push(byte x, byte y, int lvl) {
  // this % business is here to avoid array bounds checks
  // there can't be more than 32 moves and there can't be more than 32 ply searches
  // as it is 20 ply takes minutes and 21 ply would take an hour... 32 ply is not
  // happening in our universe.  By comparison in 1987 we could do 12 ply at end game.
  // So it's grown to 20 in like 36 years.
  Stack S = stacks[lvl];
  int top = S.top % 32;
  S.xy[top] = (ushort)(x<<8 | y);
  S.top++;
}

// and they come off... the order is arbitrary anyway and stack is cheap
// so we do that
bool pop_move(out byte x, out byte y, byte lvl)
{
  Stack S = stacks[lvl % 32];
  if (S.top > 0) {
    S.top--;
    int top = S.top % 32;
    ushort xy = S.xy[top];
    x = (byte)(xy >> 8);
    y = (byte)(xy);
    return true;
  }

  x = y = 0xff;
  return false;
}


const int HORRIBLE = -32000;
const int  GREAT = 32000;

int boards;
double total_time;
bool searching_to_end = false;
byte bx;
byte by;
int  bs;
bool IRQ;


void print_duration(double duration) {
  int seconds = (int)duration;
  int minutes = seconds/60;
  int millis = (int)((duration - seconds) * 1000);
  seconds %= 60;
  Console.Write("{0}:{1:D2}.{2:D3}", minutes, seconds, millis);
}

Timer? timer;

void alarm(int timeoutInSeconds) {
  if (timeoutInSeconds == 0) {
    if (timer != null) {
      timer.Dispose();
      timer = null;
    }
    return;
  }

  timer = new Timer((state) =>
  {
      // This code will be executed when the timer elapses
      // Console.WriteLine("Timeout occurred!");
      IRQ = true;
      if (timer != null) {
        timer.Dispose(); // Dispose of the timer
        timer = null;
      }
  }, null, timeoutInSeconds * 1000, Timeout.Infinite);
}

int search(BOARD board, byte is_white, out byte bestx, out byte besty)
{
  int lvl;
  byte start;
  bool found_anything;
  int limit;

  if (turn < 15)
    limit = 2;
  else if (turn < 30)
    limit = 4;
  else
    limit = 10;

  boards = 0;
  searching_to_end = false;
  IRQ = false;

  if (turn <= ENDGAME) {
    alarm(limit);
    start = 2;
  } else {
    Console.WriteLine("Seeking to end of board\n");
    start = (byte)(64 - turn);
    if (start % 2 != 0) {
      start++;
    }
  }

  Stopwatch stopwatch = new Stopwatch();
  stopwatch.Start();

  try {
    found_anything = valid(board, is_white, start);
    if (!found_anything) {
      bx = 0xff;
      by = 0xff;
      bs = HORRIBLE;
      goto no_moves;
    }

    reset_scored_moves(0);
    while (pop_move(out bestx, out besty, start)) {
      insert_scored_move(bestx, besty, 0, 0);
    }

    lvl = 0;
    for (byte i = start; i < 65; i += 2) {
      Console.Write("{0,2:D}: ", i);
      rsearch(board, is_white, i, lvl);
      if (i + 2 > 64 - turn)
        break;
      lvl = 1 - lvl; // alternate between 0 and 1
    }

    no_moves:
    alarm(0);
  }
  catch (TimeoutException) {
    alarm(0);
  }

  bestx = bx;
  besty = by;
  stopwatch.Stop();

  double duration = stopwatch.ElapsedMilliseconds / 1000.0;
  if (duration == 0.0) { duration= 0.001; }
  total_time += duration;

  Console.Write("\nEvaluated {0:N0} boards in ", boards);
  print_duration(duration);
  Console.Write(" {0:N0} boards/sec  Total time used: ", (int)(boards/duration));
  print_duration(total_time);
  Console.WriteLine("");

  return bs;
}


int score(BOARD board, byte is_white)
{
  byte other = (byte)(1-is_white);
  var me = board.half(is_white);
  var him = board.half(other);

  int s = 0;

  if (turn > ENDGAME) {
    for (int j = 0; j < 8; j++) {
      s += bit_count[me[j]];
    }
    return s;
  }

  int i;
  for (i = 2; i < 6; i++) {
    s += weighted_row_value[me[i]];
  }

  s -= bit_count[me[1] & 0x7e] + bit_count[me[6] & 0x7e] +
       ((bit_count[me[1] & 0x42] + bit_count[me[6] & 0x42]) << 2);

  int t = 0;
  for (i = 0; i < 8; i++)
    t = (t << 1) | (1 & (me[i] >> 7));

  for (i = 0; i < 8; i++) {
    t = (t << 1) | (1 & (him[i] >> 7));
  }

  s += edge[t];

  t = 0;
  for (i = 0; i < 8; i++)
    t = (t << 1) | (me[i] & 1);
  for (i = 0; i < 8; i++)
    t = (t << 1) | (him[i] & 1);

  s += edge[t] + edge[(me[0] << 8) | him[0]] + edge[(me[7] << 8) | him[7]];

  return s;
}

int rsearch(BOARD board, byte is_white, byte depth, int lvl)
{
  int sc;
  byte x;
  byte y;
  byte other = (byte)(1-is_white);

  reset_scored_moves(1 - lvl);
  searching_to_end = false;


  if (!remove_scored_move(out x, out y, lvl)) {
    return 0;
  }

  Console.Write("{0}{1}=", (char)(x + 'a'), (char)('8' - y));
  var brd = board.clone();
  flip(brd, is_white, x, y);

  bs = mini(brd, other, (byte)(depth - 1), HORRIBLE, GREAT);
  bx = x;
  by = y;
  Console.Write("{0}  ", bs - ((turn > ENDGAME) ? 0 : SCORE_BIAS));

  insert_scored_move(x, y, bs, 1-lvl);

  while (remove_scored_move(out x, out y, lvl)) {
    Console.Write("{0}{1}", (char)(x + 'a'), (char)('8' - y));
    brd = board.clone();
    flip(brd, is_white, x, y);
    sc = mini(brd, other, (byte)(depth - 1), bs, GREAT);
    insert_scored_move(x, y, sc, 1-lvl);
    if (sc > bs) {
      Console.Write('=');
      bx = x;
      by = y;
      bs = sc;
    } else {
      Console.Write('<');
    }

    Console.Write("{0}  ", sc - ((turn > ENDGAME) ? 0 : SCORE_BIAS));
  }
  Console.WriteLine("");
  return bs;
}

// This is keeping sorted scores, there are two sets of scores

struct scored_move {
  public int score;
  public byte x;
  public byte y;
};

// note that this list is small, like if there are 10 valid scored_moves that's a lot
// the size is 64 because that's how many squares there are on the board
// and that's still small but it we can't really have 64 valid scored_moves
class scored_move_list {
  public byte _put; // the number we put in
  public byte _get; // the one to get next
  public scored_move[] moves;

  public scored_move_list() { 
    moves = new scored_move[32];
  }
};

// we alternate between two
class SList {
  public scored_move_list a;
  public scored_move_list b;

  public SList() {
    a = new scored_move_list();
    b = new scored_move_list();
  }
}

SList slist = new SList();

// reset the count of scored_moves in this level
// lvl is 0/1 corresponding to the current recursion level, it alternates
// so we're reading off of lvl and writing onto !lvl at any moment
void reset_scored_moves(int lvl)
{
  scored_move_list l = lvl == 0 ? slist.a : slist.b;
  l._get = 0;
  l._put = 0;
}

void insert_scored_move(byte x, byte y, int score, int lvl)
{
  scored_move_list l = lvl == 0 ? slist.a : slist.b;

  int i;

  // find the place to insert this scored_move
  // stop at the first place where this score is bigger
  // (i.e. the best ends up at the front)
  for (i = 0; i < l._put; i++) {
    if (score > l.moves[i&31].score) {
      break;
    }
  }

  for (int j = l._put; j > i; j--) {
    l.moves[j] = l.moves[j-1];
  }

  l.moves[i].x = x;
  l.moves[i].y = y;
  l.moves[i].score = score;
  l._put++;
}

bool remove_scored_move(out byte x, out byte y, int lvl)
{
  scored_move_list l = lvl == 0 ? slist.a : slist.b;

  if (l._get >= l._put) {
    x = y = 0xff;
    return false;
  }

  int g = l._get & 31;

  x = l.moves[g].x;
  y = l.moves[g].y;
  l._get++;

  return true;
}


// minimax search with alpha beta pruning
int mini(BOARD board, byte is_white, byte depth, int a, int b)
{
  byte other = (byte)(1-is_white);
  if (IRQ) {
    throw new TimeoutException("Operation timed out.");
  }
  boards++;
  if (depth == 0) {
    return score(board, is_white);
  }
  else {
    int sc;
    reset_move_stack(depth);

    bool found_anything = valid(board, is_white, depth);
    if (!found_anything) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = true;
        sc = maxi(board, other, (byte)(depth + 1), a, b);
      } else {
        sc = maxi(board, other, (byte)(depth - 1), a, b);
      }
      return sc;
    }
    searching_to_end = false;

    byte x;
    byte y;
    while (pop_move(out x, out y, depth)) {
      var brd = board.clone();
      flip(brd, is_white, x, y);
      sc = maxi(brd, other, (byte)(depth - 1), a, b);

      // in this pass we're minning the maxes
      if (sc < b) {
        // so this says we found a new min
        b = sc;

        // if this happens then that means this min will
        // be even worse than a previous known min
        // from the max pass above us
        // that means the max pass will not use this
        // score for sure it's too low so just stop
        // computing, we can prune here.
        if (b <= a) {
          return b;
        }
      }
    }
    return b;
  }
}

int maxi(BOARD board, byte is_white, byte depth, int a, int b)
{
  byte other = (byte)(1-is_white);

  if (IRQ)
    throw new TimeoutException("Operation timed out.");
  boards++;
  if (depth == 0) {
    return score(board, is_white);
  }
  else {
    int sc;
    reset_move_stack(depth);

    bool found_anything = valid(board, is_white, depth);
    if (!found_anything) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = true;
        sc = mini(board, other, (byte)(depth + 1), a, b);
      } else {
        sc = mini(board, other, (byte)(depth - 1), a, b);
      }
      return sc;
    }
    searching_to_end = false;

    byte x;
    byte y;
    while (pop_move(out x, out y, depth)) {
      var brd = board.clone();
      flip(brd, is_white, x, y);
      sc = mini(brd, other, (byte)(depth - 1), a, b);

      // in this pass we're maxxing the mins
      if (sc > a) {
        // so this says we found a new max
        a = sc;

        // if this happens then that means this max will
        // be even better than a previous known max
        // from the min pass above us
        // that means the min pass will not use this
        // score for sure it's too high so just stop
        // computing, we can prune here.
        if (a >= b)
          return a;
      }
    }
    return a;
  }
}

// the current depth just tell us which stack to put the valid moves on
// the stacks are all pre-allocated so there is no malloc
bool valid(BOARD board, byte is_white, byte current_depth) {
  byte other = (byte)(1-is_white);
  var me = board.half(is_white);
  var him = board.half(other);

  reset_move_stack(current_depth);
  bool found_anything = false;

  for (byte y = 0; y < 8; y++) {
    ushort row = (ushort)((me[y] << 8) | him[y]);
    byte used = (byte)(row | (row >> 8));

    // already full on this row, nothing to do
    if (used == 0xff)
      continue;

    byte initial_used = used;
    byte mask = 1;
    for (byte i = 0; i < 8; i++, mask <<= 1) {
      // if the current spot is occupied, skip it, no valid move here
      if ((initial_used & mask) != 0)
        continue;

      // flipt[base_3_row_index][i] tells you what the state is of the
      // row if you were to place a piece at column i.  So this says
      // if the effect of placing a piece at column i is only that the
      // piece at column i appears, then nothing flips, so it's not valid.
      // remember me is in the high bits and him is in the low bits
      // so we place onto the high bits.  And d1 has the current bit mask

      if ((row | (mask << 8)) != flipt[row, i]) {
        push(i, y, current_depth);
        used |= mask;
        found_anything = true;
      }
    }

    // if we found valid moves everywhere on this row nothing further to do
    // there couldn't be any more valid moves anyway.
    if (used == 0xff)
      continue;

    // We think of the matching as a regular expression
    // and build the state machine to match it. If E is enemy, B
    // is blank and M is mine then the match pattern is BH+M.
    //
    // Starting empty (state 0) then finding enemy piece (state 1)
    // the a piece of mine (state 3 if found), once the match fails
    // we go to state 2 and stay there. Actually we speed this up
    // by starting in state 2 if the current spot is occupied and
    // then only reading rows from there.  The machine is below:
    //
    // input alphabet:
    //
    // blank  = 0
    // enemy  = 1
    // me     = 2
    // ignore = 3  this doesn't happen it's empty, black, or white, not both
    //
    // state transitions:
    //
    // X indicates don't care because it can't happen
    // so we can pick whatever is convenient
    //
    // state is represented in two bits y0 (lo) and y1 (hi)
    // alpha is represented in two bits d0 (lo) and d1 (hi)
    //
    //                  h m
    //          input 0 1 2 3   00 01 10 x
    // state 0:       2 1 2 X   10 01 10 x
    // state 1:       3 1 2 X   11 01 10 x
    // state 2:       2 2 2 X   10 10 10 x
    // state 3:       3 3 3 X   11 11 11 x  VALID MOVE END STATE

    // y0 transitions all bits in karnaugh map order
    //                          BEST FREE CHOICE
    //           00 01 11 10    00 01 11 10
    // state 00:  0  1  x  0     0  1  1  0
    // state 01:  1  1  x  0     1  1  1  0
    // state 11:  1  1  x  1     1  1  1  1 VALID MOVE END STATE
    // state 10:  0  0  x  0     0  0  0  0
    //
    // we see that we can cover the true cases with:
    //  a row  y0 & y1
    //  a box  y0 & d1
    //       these two are compose to y0 & (y1|d1)
    //  a box  ~y1 & d0
    //
    //  net expression:  y0 = (~y1 & d0) | (y0 & (y1|d1))
    //
    // y1 transitions all bits in karnaugh map order
    //
    //                         BEST FREE CHOICE
    //           00 01 11 10    00 01 11 10
    // state 00:  1  0  x  1     1  0  1  1
    // state 01:  1  0  x  1     1  0  1  1
    // state 11:  1  1  x  1     1  1  1  1  VALID MOVE END STATE
    // state 10:  1  1  x  1     1  1  1  1
    //
    // we can cover those cases with
    // bottom half : y1
    // right  half : d1
    // first and last column:  ~d0 (the old wrap around trick)
    //
    // net expression y1 = y1 | d1 | ~d0
    // i.e.  y1 |= d1 | ~d0
    //
    // Those are exactly the binary operations below to run the state machine
    //
    // To find the valid bits, those are state3 or 11 so we just do y0 &= y1
    // any bits that are set are valid moves.
    //
    // I'm writing this comment 36 years after I wrote this code
    // and I'm stunned that it worked out on the first go...


    // all used bits start in state 2 -> no match found
    // we never leave state 2 (see above)
    uint y0 = 0;
    uint y1 = (uint)(used | (used << 8) | (used << 16));

    for (int i = y - 1; i >= 0; i--) {
      byte h = him[i];
      byte m = me[i];
      uint d0 = (uint)(((byte)(h >> (y-i)) << 16) | ((byte)(h << (y-i)) << 8) | h);
      uint d1 = (uint)(((byte)(m >> (y-i)) << 16) | ((byte)(m << (y-i)) << 8) | m);

      y0 = (~y1 & d0)  | (y0 & (y1|d1));
      y1 |= d1 | ~d0;
      if (0 == ~y1) break;
    }
    y0 &= y1;
    y0 |= y0 >> 8;
    y0 |= y0 >> 8;
    byte found = (byte)y0;
    byte used2 = (byte)(used | found);

    y1 = (uint)(used2 | (used2 << 8) | (used2 << 16));
    y0 = 0;
    for (int i = y + 1;i < 8; i++) {
      byte h = him[i];
      byte m = me[i];
      uint d0 = (uint)(((byte)(h >> (i-y)) << 16) | ((byte)(h << (i-y)) << 8) | h);
      uint d1 = (uint)(((byte)(m >> (i-y)) << 16) | ((byte)(m << (i-y)) << 8) | m);

      y0 = (~y1 & d0)  | (y0 & (y1|d1));
      y1 |= d1 | ~d0;
      if (0 == ~y1) break;
    }

    y0 &= y1;
    y0 |= y0 >> 8;
    y0 |= y0 >> 8;
    found |= (byte)y0;
    found &= (byte)~used;

    // bit_values has one bit number in each nibble
    // this saves us from looking for all 8 bits
    // when often there is only 1 bit set
    if (found != 0) {
      ulong bits = bit_values[found];
      while (bits != 0) {
        byte x = (byte)(bits & 0x7);
        bits >>= 4;
        push(x, y, current_depth);
        found_anything = true;
      }
    }
  }

  return found_anything;
}

}


