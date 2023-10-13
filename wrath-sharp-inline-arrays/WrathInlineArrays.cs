using System;
using System.Diagnostics;
using System.Numerics;
using System.Reflection;
using System.Reflection.Metadata;
using System.Runtime.Intrinsics;
using Microsoft.Win32.SafeHandles;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


sealed class Othello {

int turn = 0;
int consecutive_passes = 0;
byte is_white_turn = 1;

readonly int ENDGAME = 44;
readonly int SCORE_BIAS = 8187;
readonly byte INITIAL_DEPTH = 0;
readonly bool BLACK = false;
readonly bool WHITE = true;

// this gives the value of an edge
// you have to flip the vertical bits
static x65536<ushort> edge;

// flip_table[row][x] tell us how to flip
// the row if we place a piece on
// the given row, in column x.  You have
// to extract the columns and diagonals
// to use this elsewhere
static x65536<x8<ushort>> flip_table;

// This tells us how many bits are set
// in this byte.  Useful for endgame
// scoring and current score display.
static x256<byte> bit_count;

// this gives the value of a row I own
// in the middle of the board (not the edges)
static x256<byte> weighted_row_value;

// This has the bit numbers lie 0x05 encodes to 0xc8
// one nibble encodes each bit position the high bit is set
// to make the nibble non-zero if there is a bit there
// so you can read out the bits with &7 then >>4
static x256<ulong> bit_values;

public class TimeoutException : Exception
{
    public TimeoutException(string message) : base(message)
    {
    }
}

[InlineArray(6561)]
struct x6561<T> {
  public T _0;
}

[InlineArray(65536)]
struct x65536<T> {
  public T _0;
}

[InlineArray(256)]
struct x256<T> {
  public T _0;
}

[InlineArray(32)]
struct x32<T> {
  public T _0;
}

[InlineArray(16)]
struct x16<T> {
  public T _0;
}

[InlineArray(8)]
struct x8<T> {
  public T _0;
}

[InlineArray(2)]
struct x2<T> {
  public T _0;
}

struct BOARD {
  public x2<x8<byte>> data;

  public BOARD() {
  }

  public byte get(byte is_white, int j) {
    return data[is_white & 1][j&7];
  }

  public void put(byte is_white, int j, byte val) {
    data[is_white & 1][j&7] = val;
  }

  public x8<byte> half(byte is_white) {
    return data[is_white & 1];
  }
}


// the computer provides "input" for the next move
// it's done uniformly like this so that the logic for
// player vs. computer is basically the same as computer
// vs. itself.
void computer_input(ref BOARD board, byte is_white)
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

    move(ref board, is_white, x, y);
    consecutive_passes = 0;
  }
}

// here we ask the user what they want to do.
bool user_input(ref BOARD board, byte is_white)
{
again:;
  // user input x and y
  int user_x = -1;
  int user_y = -1;

  reset_move_stack(INITIAL_DEPTH);

  // recompute the valid moves and put them on the stack
  // they go on stack number INITIAL_DEPTH (i.e. the root)
  valid(ref board, is_white, INITIAL_DEPTH);

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
      move(ref board, is_white, x, y);
      consecutive_passes = 0;
      return false;
    }
  }

  Console.WriteLine("You can't move to {0}{1}.\n", (char)(user_x + 'a'), (char)('8' - user_y));
  goto again;
}


void move(ref BOARD board, byte is_white, int x, int y)
{
  flip(ref board, is_white, x, y);
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

BOARD initial;

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
      // get no weight because there is a separate edge table
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
      flip_table[row][i] = row;
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
      flip_table[row_index][i] = row;
      continue;
    }

    // we will flip the this cell to black and white
    count += 2;

    // ok we make a copy of the row and use our flip edge helper
    ushort tmp = row;
    tmp = flip_edge_one_way(tmp, BLACK, i, 1);  // flip right
    tmp = flip_edge_one_way(tmp, BLACK, i, -1); // flip left

    // record the result of flipping
    // the flip table is normalized for black to move but
    // remember this is all me/him so in context the bits could
    // be black or white
    flip_table[row_index][i] = tmp;

    // now score the other outcome, WHITE gets the cell
    ushort t2 = row;
    t2 = flip_edge_one_way(t2, WHITE, i, 1);
    t2 = flip_edge_one_way(t2, WHITE, i, -1);

    // now add the scores of the two possible outcomes to the total
    sum += edge_recursive(tmp) + edge_recursive(t2);
  }

  // the score is the average outcome
  edge[row] = (ushort)(sum / count);
  return edge[row];
}

// This helper is used to compute the overall flip table.  This is
// just doing flips along one row in one direction.  When we do the
// full flip we will use the table created by this on flipping row
// directly but we will also convert the vertical and diagonal slices
// into a virtual "row" and flip them too.  So this ends up driving
// all the flips.  We do this so that we don't have to do the expensive
// nested loop business for each row/column/diagonal when we really flip.
ushort flip_edge_one_way(ushort row, bool is_white, int x, int dx)
{
  byte him = (byte)row;
  byte me = (byte)(row >> 8);
  int x0 = x;

  // normalize black/white to me/him
  if (is_white) {
    byte tmp = him;
    him = me;
    me = tmp;
  }

  // note that we do not check if the current square is unoccupied
  // this is because there are many flip directions and any one of them
  // may have already filled in the current square.

  // if the adjacent square isn't an enemy square no flip is possible
  // in this direction, early out.
  if ((him >> (x + dx) & 1) == 0) {
    goto done;
  }

  // loop across the board advancing x by dx, the dx exit is the one
  // we will always take.
  for (;;) {
    x += dx;
    if (x < 0 || x > 7) {
      break;
    }

    // if we find our own piece there is a flip here.
    if ((1 & (me >> x)) != 0) {

      // to execute the flip we go backwards until the
      // starting square, we remove the enemies pieces
      // and replace them with our own pieces
      x -= dx;
      while (x != x0) {
        me |= (byte)(1 << x);    // turn on me
        him &= (byte)~(1 << x);  // turn off him
        x -= dx;
      }

      // we're done
      break;
    }

    // we can keep trying to find a flip as long as we see enemies
    if ((1 & (him >> x)) == 0) {
      break;
    }
  }

done:
  // In any case, we fill the square with our piece.  Note that we
  // do this even though there wasn't a flip maybe because the
  // we presume the move was valid for other reasons (maybe a flip
  // in the other direction).  This is only half the flip.
  me |= (byte)(1 << x0);

  // swap these back for black/white
  if (is_white) {
    byte tmp = him;
    him = me;
    me = tmp;
  }
  return (ushort)((me << 8) | him);
}

// normalize black/white to me/him as usual, convert to ref
void flip(ref BOARD board, byte is_white, int x, int y)
{
    byte other = (byte)(1-is_white);
    flip2(ref board.data[is_white], ref board.data[other], x, y);
}

// This does the actually flipping needed if a piece is placed at x, y
// We do not validate that it is legal to do so, that has already
// happened.
void flip2(ref x8<byte> me, ref x8<byte> him, int x, int y)
{
  // extract the bits for horizontal flipping and apply the flip table
  // Note that the flip table assumes the target square (x, y) is empty
  // so all the helpers normalize to empty.  We might be mid-flip
  // in the late phases and the "move" step already set the bits
  // (this could be changed by tweaking how we build flip_table)
  ushort row = gethorz(ref me, ref him, x, y);

  ushort new_row = flip_table[row][x];
  puthorz(ref me, ref him, y, new_row);

  // pull the vertical bits into a "row" and flip it, then put it back
  row = getvert(ref me, ref him, x, y);
  new_row = flip_table[row][y];

  // it's expensive to write vertical, so don't do it if nothing changed
  // remember we normalize back to "the square is filled by me" to do
  // this test.  
  row |= (ushort)(0x100 << y);
  if (new_row != row) {
    putvert(ref me, ref him, x, new_row);
  }

  // now the first diagonal direction (x and y both increasing)
  // same optimization
  row = getdiag1(ref me, ref him, x, y);
  new_row = flip_table[row][x];
  row |= (ushort)(0x100 << x);
  if (new_row != row)
    putdiag1(ref me, ref him, x, y, new_row);

  // finally the second diagonal direction (x increasing and y decreasing)
  // same optimization
  row = getdiag2(ref me, ref him, x, y);
  new_row = flip_table[row][x];
  row |= (ushort)(256 << x);
  if (new_row != row)
    putdiag2(ref me, ref him, x, y, new_row);
}

ushort gethorz(ref x8<byte> me, ref x8<byte> him, int x, int y)
{
  // pull the row out in the usual way and strip the "me" bit at column x
  // we don't have to normalize to x,y OFF because we are first
  return (ushort)((me[y] << 8) | him[y]);
}

void puthorz(ref x8<byte> me, ref x8<byte> him, int y, ushort row)
{
  // super easy to put horizontal row back
  me[y] = (byte)(row >> 8);
  him[y] = (byte)(row & 0xff);
}

ushort getvert(ref x8<byte> me, ref x8<byte> him, int x, int y)
{
  // these are the starting output bits
  ushort row = (ushort)(((him[0] >> x) & 1));
  row |= (ushort)(((him[1] >> x) & 1) << 1);
  row |= (ushort)(((him[2] >> x) & 1) << 2);
  row |= (ushort)(((him[3] >> x) & 1) << 3);
  row |= (ushort)(((him[4] >> x) & 1) << 4);
  row |= (ushort)(((him[5] >> x) & 1) << 5);
  row |= (ushort)(((him[6] >> x) & 1) << 6);
  row |= (ushort)(((him[7] >> x) & 1) << 7);
  row |= (ushort)(((me[0] >> x) & 1) << 8);
  row |= (ushort)(((me[1] >> x) & 1) << 9);
  row |= (ushort)(((me[2] >> x) & 1) << 10);
  row |= (ushort)(((me[3] >> x) & 1) << 11);
  row |= (ushort)(((me[4] >> x) & 1) << 12);
  row |= (ushort)(((me[5] >> x) & 1) << 13);
  row |= (ushort)(((me[6] >> x) & 1) << 14);
  row |= (ushort)(((me[7] >> x) & 1) << 15);

  // normalize to x, y OFF, previous flip directions may have set this
  row &= (ushort)~(0x100 << y);
  return row;
}

void putvert(ref x8<byte> me, ref x8<byte> him, int x, ushort row)
{
  // this time we will write out the x column so that it matches the
  // bits the row, reversing what getvert does.
  byte mask_out = (byte)(1 << x);

  // flip the bits that need flipping
  him[0] ^= (byte)(mask_out & (him[0] ^ ((row & 1) << x))); row >>= 1;
  him[1] ^= (byte)(mask_out & (him[1] ^ ((row & 1) << x))); row >>= 1;
  him[2] ^= (byte)(mask_out & (him[2] ^ ((row & 1) << x))); row >>= 1;
  him[3] ^= (byte)(mask_out & (him[3] ^ ((row & 1) << x))); row >>= 1;
  him[4] ^= (byte)(mask_out & (him[4] ^ ((row & 1) << x))); row >>= 1;
  him[5] ^= (byte)(mask_out & (him[5] ^ ((row & 1) << x))); row >>= 1;
  him[6] ^= (byte)(mask_out & (him[6] ^ ((row & 1) << x))); row >>= 1;
  him[7] ^= (byte)(mask_out & (him[7] ^ ((row & 1) << x))); row >>= 1;

  me[0] ^= (byte)(mask_out & (me[0] ^ ((row & 1) << x))); row >>= 1;
  me[1] ^= (byte)(mask_out & (me[1] ^ ((row & 1) << x))); row >>= 1;
  me[2] ^= (byte)(mask_out & (me[2] ^ ((row & 1) << x))); row >>= 1;
  me[3] ^= (byte)(mask_out & (me[3] ^ ((row & 1) << x))); row >>= 1;
  me[4] ^= (byte)(mask_out & (me[4] ^ ((row & 1) << x))); row >>= 1;
  me[5] ^= (byte)(mask_out & (me[5] ^ ((row & 1) << x))); row >>= 1;
  me[6] ^= (byte)(mask_out & (me[6] ^ ((row & 1) << x))); row >>= 1;
  me[7] ^= (byte)(mask_out & (me[7] ^ ((row & 1) << x)));
}

// get the first diagonal, this is where y goes up when x goes up
ushort getdiag1(ref x8<byte> me, ref x8<byte> him, int x, int y)
{
  int x0 = 0;
  int y0 = y - x;
  int y1 = 7;

  if (y0 < 0) {
    x0 = -y0;
    y1 = 7 - x0;
    y0 = 0;
  }

  byte mask = (byte)(1 << x0);
  ushort row = 0;

  for (int i = y0; i <= y1; i++, mask <<= 1) {
    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= (ushort)(((me[i] & mask) << 8) | (him[i] & mask));
  }

  // normalize to x, y OFF, previous flip directions may have set this
  return (ushort)(row & ~(0x100 << x));
}

// write back the first diagonal, this is where y goes up when x goes up
void putdiag1(ref x8<byte> me, ref x8<byte> him, int x, int y, int row)
{
  int x0 = 0;
  int y0 = y - x;
  int y1 = 7;

  if (y0 < 0) {
    x0 = -y0;
    y1 = 7 - x0;
    y0 = 0;
  }

  byte mask = (byte)(1 << x0);
  byte hi = (byte)(row >> 8);

  for (int i = y0; i <= y1; i++, mask <<= 1) {
    // flip the bits that need flipping
    me[i] ^= (byte)((hi ^ me[i]) & mask);
    him[i] ^= (byte)((row ^ him[i]) & mask);
  }
}

// get the second diagonal, this is where y goes down when x goes up
ushort getdiag2(ref x8<byte> me, ref x8<byte> him, int x, int y)
{
  int x0 = 0;
  int y0 = x + y;
  int y1 = 0;

  if (y0 > 7) {
     x0 = y0 - 7;
     y1 = x0;
     y0 = 7;
  }

  byte mask = (byte)(1 << x0);
  ushort row = 0;

  for (int i = y0; i >= y1; i--, mask <<= 1) {
    // merge in the appropriate column from the appropriate row
    // mask and y_diag do exactly this...  me bits go in the high byte.
    row |= (ushort)(((me[i] & mask) << 8) | (him[i] & mask));
  }

  // normalize to x, y OFF, previous flip directions may have set this
  return (ushort)(row & ~(0x100 << x));
}

void putdiag2(ref x8<byte> me, ref x8<byte> him, int x, int y, ushort row)
{
  int x0 = 0;
  int y0 = x + y;
  int y1 = 0;

  if (y0 > 7) {
     x0 = y0 - 7;
     y1 = x0;
     y0 = 7;
  }

  byte mask = (byte)(1 << x0);
  byte hi = (byte)(row >> 8);

  for (int i = y0; i >= y1; i--, mask <<= 1) {
    // flip the bits that need flipping
    me[i] ^= (byte)((hi ^ me[i]) & mask);
    him[i] ^= (byte)((row ^ him[i]) & mask);
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

  initial.data[1][3] = (byte)0x10;
  initial.data[1][4] = (byte)0x08;
  initial.data[0][3] = (byte)0x08;
  initial.data[0][4] = (byte)0x10;

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
      user_input(ref initial, is_white_turn);
    else
      computer_input(ref initial, is_white_turn);

    display_score(initial);
    is_white_turn = (byte)(1 -is_white_turn);

    if (test_mode) {
      Console.WriteLine("test complete\n");
      Environment.Exit(0);
    }
  }
}

// Produces this:
//
// - - B B - W - -
// B - B B B B - B
// B B W W W W B B
// B W B W W B W B
// - W W B B W W -
// W W W W W W W W
// - - - W B B - -
// - - W - B B - -
//
// b to play
//
// Which is useful because it's super easy to make one by hand
// The "load" function reads this same format.
// Returns true if the save aborted.
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
sealed class Stack {
  public byte top;
  public x32<ushort> xy;
}

// these are all the stacks we will ever need, no malloc
Stack[] stacks = new Stack[32];

void reset_move_stack(int lvl)
{
  stacks[lvl].top = 0;
}

// each next valid move at this recursion level is pushed on its own stack
void push_move(byte x, byte y, int lvl) {
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

  // This is some rudimentry time management.  It turns out
  // that it's not very interesting to spent a lot of time
  // thinking about the early moves.  First they don't matter
  // much but second, in the old days the code easily got
  // to 6 ply and had no hope of finishing 8 with even
  // a more generous budget. So we deem it not worth it
  // and save the seconds for later in the game when it
  // will matter more.  We're also hoarding a big budget
  // so that when we hit turn = ENDGAME we can search
  // to the end without losing because we run out of time
  // In 1987 that was about 12 ply from the end.  So
  // turn 52.  Now we can do it at Turn 44 -- 20 ply.
  // We also do it faster now.  It was good 10 minutes in 1987.
  // On a modern CPU we can do the endgame search
  // in about 90s now.
  if (turn < 15)
    limit = 2;
  else if (turn < 30)
    limit = 4;
  else
    limit = 10;

  boards = 0;
  searching_to_end = false;
  IRQ = false;

  // arm the clock if we need to
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
    // if we have no moves... we have to pass
    found_anything = valid(ref board, is_white, start);
    if (!found_anything) {
      bx = 0xff;
      by = 0xff;
      bs = HORRIBLE;
      goto no_moves;
    }

    // if we have some moves then seed the initial move list
    // normally we do these in order of goodness (best first)
    // but at this point we know nothing.  It all starts at
    // score == 0.
    reset_scored_moves(0);
    while (pop_move(out bestx, out besty, start)) {
      insert_scored_move(bestx, besty, 0, 0);
    }

    // we only resort the moves in best order at the top level
    // in principle we could do this at every level in the tree
    // but it wasn't deemed worth it.  We seem to get good value
    // out of our alpha/beta pruning just ordering the top
    // level and there's the cost of all that sorting.  It might
    // be worth it to try to order at every level some day.  For
    // now we just have the moves from the last result and
    // the ones we're working on, so we just need two levels of
    // sorted moves.
    lvl = 0;
    for (byte i = start; i < 65; i += 2) {
      Console.Write("{0,2:D}: ", i);
      rsearch(board, is_white, i, lvl);
      if (i + 2 > 64 - turn)
        break;
      lvl = 1 - lvl; // alternate between 0 and 1
    }

    // disarm the clock
    no_moves:
    alarm(0);
  }
  catch (TimeoutException) {
    alarm(0);
  }

  // save the running best into our output
  // compute the duration and make a little report
  bestx = bx;
  besty = by;
  stopwatch.Stop();

  double duration = stopwatch.ElapsedMilliseconds / 1000.0;
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
  x8<byte> me = board.half(is_white);
  x8<byte> him = board.half(other);

  int s = 0;

  if (turn > ENDGAME) {
    // endgame scoring is just the count of bits I get
    // note that this means we don't optimize for the lowest
    // possible enemy score which means we might not play
    // truly perfectly. The wiggle room is that we might
    // be able to force more empty squares with our score
    // fixed.  This actually comes up in the game in endgame.txt    
    for (int j = 0; j < 8; j++) {
      s += bit_count[me[j]];
    }
    return s;
  }

  // use the weighted value for the rows that are in "the middle"
  int i;
  for (i = 2; i < 6; i++) {
    s += weighted_row_value[me[i]];
  }

  // Note that row 1 and 6 are never counted, they get zero score,
  // except for the penalty squares below which get negative score.

  // the square one position away from the corners are considered
  // very bad indeed
  s -= bit_count[me[1] & 0x7e] + bit_count[me[6] & 0x7e] +
       ((bit_count[me[1] & 0x42] + bit_count[me[6] & 0x42]) << 2);

  // make a virtual row that consists of the last column
  int tmp = 0;
  for (i = 0; i < 8; i++)
    tmp = (tmp << 1) | (1 & (me[i] >> 7));

  for (i = 0; i < 8; i++) {
    tmp = (tmp << 1) | (1 & (him[i] >> 7));
  }

  // add the edge score
  s += edge[tmp];

  // make a virtual row that consists of the first column
  tmp = 0;
  for (i = 0; i < 8; i++)
    tmp = (tmp << 1) | (me[i] & 1);
  for (i = 0; i < 8; i++)
    tmp = (tmp << 1) | (him[i] & 1);

  // add the edge scores of the column plus first and last row
  s += edge[tmp] + edge[(me[0] << 8) | him[0]] + edge[(me[7] << 8) | him[7]];

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

  // this is a bit of defensive coding, this can't happen
  // the `search` above us already checked.  We definitely
  // have a move to consider.
  if (!remove_scored_move(out x, out y, lvl)) {
    return 0;
  }

  // try the first move, this will become our working best
  // bx and by.  The score is in bs.
  Console.Write("{0}{1}=", (char)(x + 'a'), (char)('8' - y));
  var brd = board;
  flip(ref brd, is_white, x, y);

  // we get the score by looking at the worst outcome
  // for us, the mini in minimax.  The alpha beta
  // pruning starts with no cap -- HORRIBLE, GREAT
  // we're building up the moves in the new sorted
  // order for the next deeper pass.
  bs = mini(ref brd, other, (byte)(depth - 1), HORRIBLE, GREAT);
  bx = x;
  by = y;
  Console.Write("{0}  ", bs - ((turn > ENDGAME) ? 0 : SCORE_BIAS));

  insert_scored_move(x, y, bs, 1-lvl);

  // grab the next scored move and start considering it
  // we print them as we go.
  while (remove_scored_move(out x, out y, lvl)) {
    Console.Write("{0}{1}", (char)(x + 'a'), (char)('8' - y));
    brd = board;
    flip(ref brd, is_white, x, y);
    sc = mini(ref brd, other, (byte)(depth - 1), bs, GREAT);

    // re-insert this move on the other sorted list
    // print xx=nn if this is a new best score
    // print xx<nn if it isn't.  We don't know
    // the exact score if it's lower because it
    // might have been pruned.  All we know for sure
    // is that it can't be better than what we have.
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
sealed class scored_move_list {
  public byte _put; // the number we put in
  public byte _get; // the one to get next
  public x32<scored_move> moves;

  public scored_move_list() { }
};

// we alternate between two
sealed class ScoredMoveSet {
  public scored_move_list a;
  public scored_move_list b;

  public ScoredMoveSet() {
    a = new scored_move_list();
    b = new scored_move_list();
  }
}

ScoredMoveSet scored_moves = new ScoredMoveSet();

// reset the count of scored_moves in this level
// lvl is 0/1 corresponding to the current recursion level, it alternates
// so we're reading off of lvl and writing onto !lvl at any moment
void reset_scored_moves(int lvl)
{
  scored_move_list l = lvl == 0 ? scored_moves.a : scored_moves.b;
  l._get = 0;
  l._put = 0;
}

void insert_scored_move(byte x, byte y, int score, int lvl)
{
  scored_move_list l = lvl == 0 ? scored_moves.a : scored_moves.b;

  int i;

  // find the place to insert this scored_move
  // stop at the first place where this score is bigger
  // (i.e. the best ends up at the front)
  for (i = 0; i < l._put; i++) {
    if (score > l.moves[i&31].score) {
      break;
    }
  }

  i &= 31;
  for (int j = l._put; j > i; j--) {
    j &= 31;
    l.moves[j] = l.moves[j-1];
  }

  l.moves[i].x = x;
  l.moves[i].y = y;
  l.moves[i].score = score;
  l._put++;
}

bool remove_scored_move(out byte x, out byte y, int lvl)
{
  scored_move_list l = lvl == 0 ? scored_moves.a : scored_moves.b;

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


// this is the mini part of minimax.  We're going to pick
// the worst score here (subject to alpha/beta pruning)
int mini(ref BOARD board, byte is_white, byte depth, int a, int b)
{
  byte other = (byte)(1-is_white);
  if (IRQ) {
    throw new TimeoutException("Operation timed out.");
  }
  boards++;

  // if we get to the bottom of the recursion, use the scoring function
  if (depth == 0) {
    return score(board, is_white);
  }
  else {
    int sc;
    reset_move_stack(depth);

    bool found_anything = valid(ref board, is_white, depth);
    if (!found_anything) {
      // this means there are no moves, we have to pass

      if (turn > ENDGAME && !searching_to_end) {
        // we're searching to the end, we had to pass
        // so the board didn't fill in. That means the game
        // might go one turn longer.  Continue the search
        // with one extra level so that we get to the end still.
        // This is imperfect because there could be more passes
        // along the way... oh well.  Room for improvement.
        searching_to_end = true;
        sc = maxi(ref board, other, (byte)(depth + 1), a, b);
      } else {
        // normal case, just keep scoring from here
        // we don't give ourselves a penalty for passing
        sc = maxi(ref board, other, (byte)(depth - 1), a, b);
      }
      return sc;
    }
    searching_to_end = false;

    // now process the moves, this is the mini pass
    // so we take the worst of the best
    byte x;
    byte y;
    while (pop_move(out x, out y, depth)) {
      var brd = board;
      flip(ref brd, is_white, x, y);
      sc = maxi(ref brd, other, (byte)(depth - 1), a, b);

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

// this is the maxi part of minimax.  We're going to pick
// the best score here (subject to alpha/beta pruning)
int maxi(ref BOARD board, byte is_white, byte depth, int a, int b)
{
  byte other = (byte)(1-is_white);

  if (IRQ)
    throw new TimeoutException("Operation timed out.");
  boards++;

  // if we get to the bottom of the recursion, use the scoring function
  if (depth == 0) {
    return score(board, is_white);
  }
  else {
    int sc;
    reset_move_stack(depth);

    bool found_anything = valid(ref board, is_white, depth);
    if (!found_anything) {
      // this means there are no moves, we have to pass

      if (turn > ENDGAME && !searching_to_end) {
        // we're searching to the end, we had to pass
        // so the board didn't fill in. That means the game
        // might go one turn longer.  Continue the search
        // with one extra level so that we get to the end still.
        // This is imperfect because there could be more passes
        // along the way... oh well.  Room for improvement.
        searching_to_end = true;
        sc = mini(ref board, other, (byte)(depth + 1), a, b);
      } else {
        // normal case, just keep scoring from here
        // we don't give ourselves a penalty for passing
        sc = mini(ref board, other, (byte)(depth - 1), a, b);
      }
      return sc;
    }
    searching_to_end = false;

    // now process the moves, this is the maxi pass
    // so we take the best of the worst
    byte x;
    byte y;
    while (pop_move(out x, out y, depth)) {
      var brd = board;
      flip(ref brd, is_white, x, y);
      sc = mini(ref brd, other, (byte)(depth - 1), a, b);

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

bool valid(ref BOARD board, byte is_white, byte current_depth) {
  return valid2(ref board.data[is_white & 1], ref board.data[1 & (byte)(1-is_white)], current_depth);
}

// the current depth just tell us which stack to put the valid moves on
// the stacks are all pre-allocated so there is no malloc
bool valid2(ref x8<byte> me, ref x8<byte> him, byte current_depth)
{
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

      // flip_table[row][i] tells you what the state is of the
      // row if you were to place a piece at column i.  So this says
      // if the effect of placing a piece at column i is only that the
      // piece at column i appears, then nothing flips, so it's not valid.
      // remember 'me' is in the high bits and 'him' is in the low bits
      // So we place onto the high bits, hence mask << 8.

      if ((row | (mask << 8)) != flip_table[row][i]) {
        push_move(i, y, current_depth);
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
        push_move(x, y, current_depth);
        found_anything = true;
      }
    }
  }

  return found_anything;
}

}



