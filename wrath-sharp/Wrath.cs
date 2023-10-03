using System;
using System.Numerics;
using System.Runtime.Intrinsics;

class Othello {

public class TimeoutException : Exception
{
    public TimeoutException(string message) : base(message)
    {
    }
}

struct BOARD {
  Vector128<ulong> data;

  public BOARD(BOARD copy) {
    data = copy.data;
  }

  public BOARD() {
    data = Vector128<ulong>.Zero;
  }

  public BOARD(Vector128<byte> v) {
    data = v.AsUInt64();
  }

  public byte get(bool is_white, int j) {
    return data.AsByte().GetElement(j + (is_white ? 8 : 0));
  }

  public void put(bool is_white, int j, byte b) {
    data = data.AsByte().WithElement((j + (is_white ? 8 : 0)), b).AsUInt64();
  }

  public BOARD(ulong b, ulong w) {
    data = Vector128.Create(b).WithElement(1,w);
  }

  public HBOARD half(bool is_white) {
    return new HBOARD(data.GetElement(is_white ? 1 : 0));
  }
}

// this is one player, half the board.  We keep the same data type for easy copying
// to get the HBOARD we extract the top or bottom half
struct HBOARD {
  Vector128<byte> data;

  public HBOARD() {
    data = Vector128<byte>.Zero;
  }

  public HBOARD(HBOARD copy) {
    data = copy.data;
  }

  public HBOARD(ulong x) {
    data = Vector128<ulong>.Zero.WithElement(0, x).AsByte();
  }

  public byte get(int i) {
    return data.GetElement(i);
  }

  public void put(int i, byte b) {
    data = data.WithElement(i, b);
  }
}

static int turn = 0;
static int consecutive_passes = 0;
static int is_white_turn = 1;
static readonly int ENDGAME = 44;
static readonly int SCORE_BIAS = 8187;

/*
// the computer provides "input" for the next move
// it's done uniformly like this so that the logic for
// player vs. computer is basically the same as computer
// vs. itself.
static void computer_input(BOARD board, bool is_white)
{
  int x, y;

  int score = search(board, is_white, out x, out y);
  int score = 0;  x = 0; y = 0;
  if (x == -1) {
    Console.WriteLine("{0} has to pass.", is_white ? 'W' : 'B');
    consecutive_passes++;
  } else {
    // at the endgame the score is the number of pieces we have
    int score_bias = turn > ENDGAME ? 0 : SCORE_BIAS;
    Console.WriteLine("best move for {0} is at {1}{2} (score {3})", is_white ? 'W' : 'B',
           x + 'a', '8' - y, score - score_bias);
    move(board, is_white, x, y);
    consecutive_passes = 0;
  }
}
*/

static string ascii_values = "-BW?";

// This is true or false if the bit is set in the row for one color
static int RINDEX(int rowbits, int x) {
  return (rowbits & (1 << x)) != 0 ? 1 : 0;
}

// this get the row out of the given board (black or white) and then is true
// if that board has a bit at x, y
static int INDEX(HBOARD hb, int x, int y) {
  byte b = hb.get(y);
  return (b & (1 << x)) != 0 ? 1 : 0;
}

// this gets the type of the row board[0] is black and board[1] is white
static int TYPE(BOARD board, int x, int y) {
  byte b1 = board.half(false).get(y);
  byte mask = (byte)(1 << x);
  int i1 = (b1 & mask) != 0 ? 1 : 0;
  byte b2 = board.half(true).get(y);
  int i2 = (b2 & mask) != 0 ? 2 : 0;
  return i1 + i2;
}

// here row has a single row, black is the high bits and white is the low bits
// the values are 0 empty, 1 black and 2 white just like the ascii table above
static int RTYPE(int row, int x) {
   return ((row >> (8 + x)) & 1) + 2 * ((row >> x) & 1);
}

// draw all the rows of the board
static void display(BOARD board)
{
  int  x, y;

  for (y = 0; y < 8; y++) {
    // row label (digit)
    Console.Write("\t\t\t\t{0} ", (char)('8' - y));

    // board data
    for (x = 0; x < 8; x++) {
      Console.Write(ascii_values[TYPE(board, x, y)]);
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

static BOARD initial = new BOARD(
   Vector128<byte>
    .Zero
    .WithElement(3, (byte)0x10)
    .WithElement(4, (byte)0x08)
    .WithElement(11, (byte)0x08)
    .WithElement(12, (byte)0x10));


static void display_one_row(int rowbits) {
  for (int x = 0; x < 8; x++) {
    Console.Write(ascii_values[RTYPE(rowbits, x)]);
  }
}


static public void Main(string[] args) {
  display(initial);
  display_one_row(0x1824);
  build_tables();
  Console.WriteLine();
}

// this is the raw score, just counts
// the evaluation is not this at all.
static void display_score(BOARD board)
{
  int sc1, sc2, i;

  sc1 = 0;
  for (i = 0; i < 8; i++) {
    sc1 += bit_count[board.half(true).get(i)];
  }

  Console.Write("\t\t\t\t\t\t    Score: W{0} ", sc1);

  sc2 = 0;
  for (i = 0; i < 8; i++) {
    sc2 += bit_count[board.half(false).get(i)];
  }

  Console.Write("B={0}\n", sc2);
  turn = sc1 + sc2;
}


static void build_lookups()
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

static byte[] bit_count = new byte[256];
static byte[] weighted_row_value = new byte[256];
static ulong[] bit_values = new ulong[256];


// The black and white occupied slots of a board row
// are represented by two bytes of a short. But there
// are only 3 combinations, none, black, and white.
// This means you can represent a given board row as
// a base three number.  pack_table[i] is the pre-computed
// number representing the row as computed by pack_board_row.
// So literally pack_table[mask] == pack_board_row(mask)
// for all valid masks.  We skip any that have both black
// and white set.
static ushort[] pack_table = new ushort[65536];

static int pack_board_row(int board_row)
{
  int s = 0;

  for (int mask = 0x01; mask <= 0x80; mask <<= 1) {
    // s += (s <<1 ) multiplies s by 3 by adding it to twice itself
    // then either add 1 or 2 depending on which bit is set
    s += (s << 1) + ((board_row & mask) != 0 ? 1 : 0) + ((((board_row & (mask << 8)) != 0 ? 1 : 0)) << 1);
  }

  return s;
}

// This is only used for testing... see the test code below
static int unpack_board_row(int packed_value)
{
  int d;
  int board_row = 0;

  for (int mask = 0x80; mask != 0; mask >>= 1) {
    // pull out the first base 3 digit
    // note that the board high bits are stored in the lowest digit
    d = packed_value % 3;

    // set up to pull out the next digit
    packed_value /= 3;

    // if the digit is zero set nothing
    if (d != 0) {
      if (d == 1) {
        // if the digit is 1 set the low bits
        board_row |= mask;
      }
      else {
        // if the digit is 2 set the high bits
        board_row |= (mask << 8);
      }
    }
  }

  return board_row;
}


static ushort[] edge = new ushort[65536];
static ushort[,] flipt = new ushort[6561, 8];

static void build_tables()
{
  Console.WriteLine("Building general lookup tables");
  build_lookups();

  Console.WriteLine("Building pack_board_row table");
  build_pack_table();

  Console.WriteLine("Building edge table");

  for (int i = 0; i < 65536; i++) {
    // only process valid row combos -- no cells that are both black and white
    if ((i & (i >> 8)) == 0) {
      edge_recursive((ushort)i);
    }
  }

  Console.WriteLine("Computation complete");
}

static readonly bool BLACK = false;
static readonly bool WHITE = true;

static int edge_recursive(ushort row)
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
      flipt[pack_table[row], i] = row;
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
  int row_index = pack_table[row];

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



// The loose row representation is 8 bits for black 8 bits for white
// but because any given cell can only have 3 actual states, black-white-empty
// the total number of valid rows is only 3^8 which is 6561.  This is much smaller
// than 2^16 (65536) almost exactly 10% the size.  So for the edge tables
// we only want to store data for valid row combos.  Hence we need a function
// that maps from a loose row to its row number.  We can do this with a lookup
// table. This code creates that table.  The code is sufficiently fast now
// that the test code is in there full time which would once have been unthinkable.
//
// A reverse mapping would be useful if we wanted to visit all row combinations
// but that basically doesn't happen.  So unpack only exists for this test code.
// You could quickly visit all 6561 rows and get their loose mappings saving you
// lots of loop iterations. But at this point we don't have that mapping anyway.
static void build_pack_table()
{
  // we loop through all 64k combos but we're going to prune away the invalid ones
  for (int row = 0; row < 65536; row++) {
    // any value where any high byte has a common bit with a low byte can be
    // skipped. It isn't a valid board row -- that would be a a row with cells that
    // are both black and white.
    if ((row & (row >> 8)) != 0) continue;

    int row_index = pack_board_row(row);

    // do the reverse mapping
    int row_copy = unpack_board_row(row_index);

    // test packing/unpacking -- paranoid testing
    if (row != row_copy) {
      Console.Write("Yipe! ");
      Console.WriteLine("{0:x} {1} {2:x} ", row, row_index, row_copy);
      display_one_row(row);
      Console.Write(" != ");
      display_one_row(row_copy);
      Environment.Exit(99);
    }

    pack_table[row] = (ushort)row_index;

    // This could easily be done if it were ever needed
    // extern unsigned short unpack_table[6561];
    // unpack_table[row_index] = row;
  }
}

// #define DATA(mask, x) ((mask) & (1 << (x)))

static ushort fe(ushort row, bool is_white, int x, int dx)
{
  int i, him, me, x0;

  him = row & 0xff;
  me = row >> 8;
  x0 = x;

  if (is_white) {
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
  if (is_white) {
    i = him;
    him = me;
    me = i;
  }
  return (ushort)((me << 8) | him);
}

static void flip(ref BOARD board, bool is_white, int x, int y)
{
  HBOARD me = board.half(is_white);
  HBOARD him = board.half(!is_white);
  
  ushort row = (ushort)(gethorz(me, him, y) & (~(256 << x)));

  ushort new_row = flipt[pack_table[row], x];
  puthorz(ref me, ref him, y, new_row);

  row = getvert(me, him, x, y);
  new_row = flipt[pack_table[row], y];
  row |= (ushort)(256 << y);
  if (new_row != row) {
    putvert(ref me, ref him, x, new_row);
  }

  row = getdiag1(me, him, x, y);
  new_row = flipt[pack_table[row], x];
  row |= (ushort)(256 << x);
  if (new_row != row)
    putdiag1(ref me, ref him, x, y, new_row);

  row = getdiag2(me, him, x, y);
  new_row = flipt[pack_table[row], x];
  row |= (ushort)(256 << x);
  if (new_row != row)
    putdiag2(ref me, ref him, x, y, new_row);
}

static ushort gethorz(HBOARD me, HBOARD him, int y)
{
  return (ushort)((me.get(y) << 8) | him.get(y));
}

static void puthorz(ref HBOARD me, ref HBOARD him, int y, ushort row)
{
  me.put(y, (byte)(row >> 8));
  him.put(y, (byte)(row & 0xff));
}

static ushort getvert(HBOARD me, HBOARD him, int x, int y)
{
  ushort row = 0;
  for (int i = 0; i < 8; i++) {
    row |= (ushort)((((me.get(i) & (1 << x))) != 0 ? 1 : 0) << (i + 8));
    row |= (ushort)((((him.get(i) & (1 << x))) != 0 ? 1 : 0) << i);
  }
  row &= (ushort)~(1 << (y + 8));
  return row;
}

static void putvert(ref HBOARD me, ref HBOARD him, int x, ushort row)
{
  byte hi = (byte)(row >> 8);
  byte mask = 1;
  byte mx = (byte)((1 << x));

  for (int i = 0; i < 8; i++, mask <<= 1) {
    byte b = me.get(i);
    if ((hi & mask) != 0)
      b |= mx;
    else
      b &= (byte)~mx;
    me.put(i, b);

    b = him.get(i);
    if ((row & mask) != 0)
      b |= mx;
    else
      b &= (byte)~mx;
    him.put(i, b);
  }
}

static ushort getdiag1(HBOARD me, HBOARD him, int x, int y)
{
  int i, d;

  d = y - x;

  ushort row = 0;
  for (i = 0; i < 8; i++) {
    if ((i + d) < 0 || (i + d) > 7)
      continue;

    row |= (ushort)(((me.get(i + d) & (1 << i)) != 0 ? 1 : 0) << (i + 8) | ((him.get(i + d) & (1 << i)) != 0 ? 1 : 0) << i);
  }
  row &= (ushort)~(1 << (x + 8));
  return row;
}

static void putdiag1(ref HBOARD me, ref HBOARD him, int x, int y, int row)
{
  int d = y - x;
  byte hi = (byte)(row >> 8);

  byte mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    if ((i + d) < 0 || (i + d) > 7)
      continue;

    byte b = me.get(i + d);
    if ((hi & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    me.put(i + d, b);

    b = him.get(i + d);
    if ((row & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    him.put(i + d, b);
  }
}

static ushort getdiag2(HBOARD me, HBOARD him, int x, int y)
{
  int d = y + x;

  ushort row = 0;
  for (int i = 0; i < 8; i++) {
    if ((d - i) < 0 || (d - i) > 7)
      continue;

    row |= (ushort)(((me.get(d - i) & (1 << i)) != 0 ? 1 : 0) << (i + 8) | ((him.get(d - i) & (1 << i)) != 0 ? 1 : 0) << i);
  }
  row &= (ushort)~(1 << (x + 8));
  return row;
}

static void putdiag2(ref HBOARD me, ref HBOARD him, int x, int y, ushort row)
{
  int d = y + x;
  byte hi = (byte)(row >> 8);
  byte mask = 1;
  for (int i = 0; i < 8; i++, mask <<= 1) {
    if ((d - i) < 0 || (d - i) > 7)
      continue;

    byte b = me.get(d - i);
    if ((hi & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    me.put(d - i, b);

    b = him.get(d - i);
    if ((row & mask) != 0)
      b |= mask;
    else
      b &= (byte)~mask;
    him.put(d - i, b);
  }
}


}

/*


static int test_mode = 0;

int main(int argc, char **argv)
{
  int i, j;
  int player, play_side;
  FILE *f;

  player = 0;
  play_side = 0;
  if (argc >= 2) {
    switch (argv[1][0]) {

    case 'w':
    case 'W':
      player = 1;
      printf("You will be playing white.\n");
      play_side = 1;
      break;

    case 'b':
    case 'B':
      player = 1;
      printf("You will be playing black.\n");
      play_side = 0;
      break;
    }

    switch (argv[1][1]) {
    case 'l':
    case 'L':
      if (argc < 3) {
        fprintf(stderr, "wrath: You must specify a file name\n");
        exit(1);
      }
      load (argv[2], initial);

      if (argv[1][2] == 't' | argv[1][2] == 'T') {
         // do one move and stop
         test_mode = 1;
      }
    }
  }

  // make the lookup tables
  build_tables();

  // start with no passes
  consecutive_passes = 0;

  // display the initial board and score
  display(initial);
  display_score(initial);

  // repeat play until there are two passes, alternating color
  while (consecutive_passes < 2) {
    if (player && color == play_side)
      user_input(initial, color);
    else
      computer_input(initial, color);

    display_score(initial);
    color = !color;

    if (test_mode) {
      printf("test complete\n");
      exit(0);
    }
  }
}
#include "board.h"

void safe_gets(char *buf, int len)
{
  char *result = fgets(buf, len, stdin);
  if (result) {
    // clobber the trailing \n
    buf[strlen(buf) - 1] = 0;
  } else {
    buf[0] = 0;
  }
}

int save()
{
  char name[80];

  printf("filename (press return to abort): ");
  fflush(stdout);
  safe_gets(name, sizeof(name));
  if (!name[0])
    return (1);

  FILE *f = fopen(name, "w");
  if (!f)
    return (1);

  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      putc(ascii_values[TYPE(initial, x, y)], f);
      putc(' ', f);
    }
    fputc('\n', f);
  }
  putc('\n', f);
  fprintf(f, "%c to play\n", color ? 'w' : 'b');
  fclose(f);
  return 0;
}

// once used as the ^C handler
static void catch ()
{
  char confirm[100];

  printf("\nreally quit (y/n)? ");
  fflush(stdout);
  safe_gets(confirm, sizeof(confirm));
  if (confirm[0] != 'y')
    return;

  printf("save game (y/n)? ");
  fflush(stdout);
  safe_gets(confirm, sizeof(confirm));
  if (!confirm[0])
    return;

  if (confirm[0] == 'y' && save())
    return;

  exit(0);
}

int score(BOARD board, int color)
{
  int i, s;
  unsigned int t;
  unsigned char *me, *him;

  me = &board[color][0];
  him = &board[!color][0];

  t = s = 0;
  if (turn > ENDGAME) {
    for (i = 0; i < 8; i++)
      s += bit_count[me[i]];
    return (s);
  }

  for (i = 2; i < 6; i++)
    s += weighted_row_value[me[i]];
  s -= bit_count[me[1] & 0x7e] + bit_count[me[6] & 0x7e] +
       ((bit_count[me[1] & 0x42] + bit_count[me[6] & 0x42]) << 2);

  for (i = 0; i < 8; i++)
    t = (t << 1) | (!!(me[i] & (1 << 7)));
  for (i = 0; i < 8; i++)
    t = (t << 1) | (!!(him[i] & (1 << 7)));

  s += edge[t];

  t = 0;
  for (i = 0; i < 8; i++)
    t = (t << 1) | (me[i] & 1);
  for (i = 0; i < 8; i++)
    t = (t << 1) | (him[i] & 1);

  s += edge[t] + edge[(me[0] << 8) | him[0]] + edge[(me[7] << 8) | him[7]];

  return (s);
}

#define HORRIBLE -32000
#define GREAT 32000

static int boards;
static double total_time;
static int searching_to_end;
static int bx, by, bs;
static int limit;
static int IRQ;


static void print_with_commas(int n) {
  if (n < 1000) {
    printf ("%d", n);
  }
  else {
    print_with_commas(n / 1000);
    printf(",%03d", n % 1000);
  }
}

static void print_duration(double duration) {
  int seconds = (int)duration;
  int minutes = seconds/60;
  int millis = (duration - seconds) * 1000;
  seconds %= 60;
  printf("%d:%02d.%03d", minutes, seconds, millis);
}

static Timer timer;

static void alarm(int timeoutInSeconds) {
  if (timeOutInSeconds == 0) {
    timer.Dispose();
    timer = null;
  }

  timer = new Timer((state) =>
  {
      // This code will be executed when the timer elapses
      Console.WriteLine("Timeout occurred!");
      IRQ = 1;
      timer.Dispose(); // Dispose of the timer
  }, null, timeoutInSeconds * 1000, Timeout.Infinite);
}

int search(BOARD board, int color, int *bestx, int *besty)
{
  int i, start, lvl, moves;

  signal(SIGALRM, timeout);

  if (turn < 15)
    limit = 2;
  else if (turn < 30)
    limit = 4;
  else
    limit = 10;

  boards = 0;
  searching_to_end = 0;
  IRQ = 0;

  clock_t start_time = clock();
  if (turn <= ENDGAME) {
    alarm(limit);
    start = 2;
  } else {
    printf("Seeking to end of board\n");
    start = 64 - turn;
    if (start % 2)
      start++;
  }

  if (setjmp(env)) {
    *bestx = bx;
    *besty = by;
    clock_t end_time = clock();
    double duration = (end_time - start_time + 0.0)/CLOCKS_PER_SEC;

    if (duration == 0.0) {
      duration = 0.000001;
    }
    total_time += duration;

    printf("\nEvaluated ");
    print_with_commas(boards);
    printf(" boards in ");
    print_duration(duration);
    printf(" (");
    print_with_commas((int)(boards / duration));
    printf(" boards/sec).  Total time used: ");
    print_duration(total_time);
    printf("\n");
    fflush(stdout);

    return bs;
  }

  if (!(moves = valid(board, color, start))) {
    bx = -1;
    by = -1;
    bs = HORRIBLE;
    goto no_moves;
  }

  reset_scored_moves(0);
  while (pop_move(bestx, besty, start)) {
    insert_scored_move(*bestx, *besty, 0, 0);
  }

  lvl = 0;
  for (i = start; i < 65; i += 2) {
    printf("%2d: ", i);
    fflush(stdout);
    rsearch(board, color, i, lvl);
    if (i + 2 > 64 - turn)
      break;
    lvl = !lvl;
  }

no_moves:
  alarm(0);
  throw new TimeoutException("Operation timed out.");
}

static int rsearch(BOARD board, int color, int depth, int lvl)
{
  int x, y, sc;
  BOARD brd;
  int moves;

  reset_scored_moves(!lvl);
  moves = 0;
  searching_to_end = 0;

  if (!remove_scored_move(&x, &y, lvl))
    return 0;

  printf("%c%c=", x + 'a', '8' - y);
  fflush(stdout);
  bcpy(brd, board);
  flip(brd, color, x, y);

  bs = mini(brd, !color, depth - 1, HORRIBLE, GREAT);
  bx = x;
  by = y;
  printf("%d  ", bs - ((turn > ENDGAME) ? 0 : SCORE_BIAS));
  fflush(stdout);
  insert_scored_move(x, y, bs, !lvl);

  while (remove_scored_move(&x, &y, lvl)) {
    printf("%c%c", x + 'a', '8' - y);
    fflush(stdout);
    bcpy(brd, board);
    flip(brd, color, x, y);
    sc = mini(brd, !color, depth - 1, bs, GREAT);
    insert_scored_move(x, y, sc, !lvl);
    if (sc > bs) {
      putchar('=');
      bx = x;
      by = y;
      bs = sc;
    } else
      putchar('<');

    printf("%d  ", sc - ((turn > ENDGAME) ? 0 : SCORE_BIAS));
    fflush(stdout);
  }
  printf("\n");
  fflush(stdout);
  return bs;
}

static int mini(BOARD board, int color, int depth, int a, int b)
{
  if (IRQ)
    throw new TimeoutException("Operation timed out.");
  boards++;
  if (!depth)
    return score(board, color);
  else {
    BOARD brd;
    int x, y, sc, yes;

    reset_move_stack(depth);

    yes = valid(board, color, depth);
    if (!yes) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = 1;
        sc = maxi(board, !color, depth + 1, a, b);
      } else
        sc = maxi(board, !color, depth - 1, a, b);
      return sc;
    }
    searching_to_end = 0;

    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, color, x, y);
      sc = maxi(brd, !color, depth - 1, a, b);
      if (sc < b) {
        b = sc;
        if (b <= a)
          return b;
      }
    }
    return b;
  }
}

static int maxi(BOARD board, int color, int depth, int a, int b)
{
  if (IRQ)
    throw new TimeoutException("Operation timed out.");
  boards++;
  if (!depth)
    return score(board, color);
  else {
    BOARD brd;
    int x, y, sc, yes;

    reset_move_stack(depth);

    yes = valid(board, color, depth);
    if (!yes) {
      if (turn > ENDGAME && !searching_to_end) {
        searching_to_end = 1;
        sc = mini(board, !color, depth + 1, a, b);
      } else
        sc = mini(board, !color, depth - 1, a, b);
      return sc;
    }
    searching_to_end = 0;

    while (pop_move(&x, &y, depth)) {
      bcpy(brd, board);
      flip(brd, color, x, y);
      sc = mini(brd, !color, depth - 1, a, b);
      if (sc > a) {
        a = sc;
        if (a >= b)
          return a;
      }
    }
    return a;
  }
}

static void bcpy(BOARD b1, BOARD b2)
{
  memcpy(b1, b2, sizeof(BOARD));
}

void move(BOARD board, int color, int x, int y)
{
  board[color][y] |= (1 << x);
  flip(board, color, x, y);
  display(board);
}

static void show(int depth, int score) {
  int i;

  for (i = 0; i < depth; i++)
    putchar('\t');

  printf("%d\n", score);
  fflush(stdout);
}
#include "board.h"

// This is keeping sorted scores, there are two sets of scores

typedef struct {
  int score;
  char x;
  char y;
} scored_move;

  // note that this list is small, like if there are 10 valid scored_moves that's a lot
  // the size is 64 because that's how many squares there are on the board
  // and that's still small but it we can't really have 64 valid scored_moves
typedef struct {
  char put; // the number we put in 
  char get; // the one to get next
  scored_move scored_moves[64];
} scored_move_list;

static scored_move_list slist[2];

// reset the count of scored_moves in this level
// lvl is 0/1 corresponding to the current recursion level, it alternates
// so we're reading off of lvl and writing onto !lvl at any moment
void reset_scored_moves(int lvl)
{
  scored_move_list *S = &slist[lvl];
  S->get = S->put = 0;
}

void insert_scored_move(int x, int y, int score, int lvl)
{
  int i, j;
  scored_move_list *S = &slist[lvl];

  // find the place to insert this scored_move
  // stop at the first place where this score is bigger
  // (i.e. the best ends up at the front)
  for (i = 0; i < S->put; i++)
    if (score > S->scored_moves[i].score)
      break;

  if (i < S->put) {
    // bulk scored_move the scored_moves above this one to make room for the new scored_move
    memmove(&S->scored_moves[i+1], &S->scored_moves[i], sizeof(scored_move)*(S->put - i));
  }

  scored_move m = { .x = x, .y = y, .score = score };

  S->scored_moves[i] = m;
  S->put++;
}

int remove_scored_move(int *x, int *y, int lvl)
{
  scored_move_list *S = &slist[lvl];

  if (S->get >= S->put) {
    *x = *y = -1;
    return 0;
  }
  
  *x = S->scored_moves[S->get].x;
  *y = S->scored_moves[S->get].y;
  S->get++;

  return 1;
}
#include "board.h"

// This is for keeping valid moves

typedef struct {
  char x;
  char y;
} xy;

// We keep moves we are considering here, this is for holding the next set of valid moves
typedef struct {
  char top;
  xy moves[64];
} stack;

// these are all the stacks we will ever need, no malloc
static stack stacks[64];

void reset_move_stack(int lvl)
{
  stack *S = &stacks[lvl];
  S->top = 0;
}

// each next valid mmove at this recursion level is pushed on its own stack
void push(int x, int y, int lvl) {
  stack *S = &stacks[lvl];
  S->moves[S->top].x = x;
  S->moves[S->top].y = y;
  S->top++;
}

// and they come off... the order is arbitrary anyway and stack is cheap
// so we do that
int pop_move(int *x, int *y, int lvl)
{
  stack *S = &stacks[lvl];
  if (S->top) {
    S->top--;
    *x = S->moves[S->top].x;
    *y = S->moves[S->top].y;
    return 1;
  }

  *x = *y = -1;
  return 0;
}
#include "board.h"

#include "board.h"

#define INITIAL_DEPTH 0

// here we ask the user what they want to do.
int user_input(BOARD board, int color)
{
again:;
  // user input x and y
  int user_x = -1;
  int user_y = -1;
  int user_pass = 0;

  reset_move_stack(INITIAL_DEPTH);

  // recompute the valid moves and put them on the stack
  // they go on stack number INITIAL_DEPTH (i.e. the root)
  valid(board, color, INITIAL_DEPTH);

  printf("Please enter a move --> ");
  fflush(stdout);

  char s[80];
  safe_gets(s, sizeof(s));

  int len = strlen(s);

  user_pass = 0;

  for (int i = 0; i < len; i++) {
    if (s[i] == '?') {
      // help
      printf("\n");
      printf("?\t\t\t\t:display this help page\n");
      printf("[a-z][1-8]\t\t\t:enter a move\n");
      printf("p\t\t\t\t:pass this turn\n");
      printf("r\t\t\t\t:redraw the board\n");
      printf("s\t\t\t\t:save current game\n");
      printf("q\t\t\t\t:quit current game\n");
      printf("\n");
      fflush(stdout);
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
      user_pass = 1;
    }
    if (s[i] == 'q') {
      // quit
      exit(0);
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
        exit(0);
      }
    }
  }

  if ((user_x == -1 || user_y == -1) && user_pass == 0) {
    printf("?syntax error\n");
    fflush(stdout);
    goto again;
  }

  if (user_pass == 1) {
    // make sure there are no moves
    int x, y;
    if (pop_move(&x, &y, INITIAL_DEPTH)) {
      printf("You can't pass unless you have no moves\n");
      fflush(stdout);
      goto again;
    }
    consecutive_passes++;
    printf("Pass accepted.\n");
    fflush(stdout);
    return 0;
  }

  // make sure that the entered move is a valid move
  int x, y;
  while (pop_move(&x, &y, INITIAL_DEPTH)) {
    if (x == user_x && y == user_y) {
      printf("Move to %c%c accepted.\n", x + 'a', '8' - y);
      fflush(stdout);
      move(board, color, x, y);
      consecutive_passes = 0;
      return 0;
    }
  }
  printf("You can't move to %c%c.\n", user_x + 'a', '8' - user_y);
  fflush(stdout);
  goto again;
}
#include "board.h"

#define tobyte(x) ((unsigned char)(x))
#define load_state(a, b, c) ((tobyte(a) << 16) | (tobyte(b) << 8) | tobyte(c))

// the current depth just tell us which stack to put the valid moves on
// the stacks are all pre-allocated so there is no malloc
int valid(BOARD board, int color, int current_depth)
{
  // we put the board in a sea of zeros so we can go off either
  // end with impunity

  uint64_t words[5];

  words[0] = 0;
  words[1] = *(uint64_t *)&board[color][0];
  words[2] = 0;
  words[3] = *(uint64_t *)&board[!color][0];
  words[4] = 0;

  uint8_t *me  = (uint8_t*)(&words[0]);
  uint8_t *him = (uint8_t*)(&words[2]);

  reset_move_stack(current_depth);
  int found_anything = 0;

  for (int y = 0; y < 8; y++) {
    unsigned row = (me[8+y] << 8) | him[8+y];
    unsigned char used = (row | (row >> 8));

    // already full on this row, nothing to do
    if (used == 0xff)
      continue;

    unsigned initial_used = used;
    unsigned mask = 1;
    for (int i = 0; i < 8; i++, mask <<= 1) {
      // if the current spot is occupied, skip it, no valid move here
      if (initial_used & mask)
        continue;

      // flipt[base_3_row_index][i] tells you what the state is of the
      // row if you were to place a piece at column i.  So this says
      // if the effect of placing a piece at column i is only that the
      // piece at column i appears, then nothing flips, so it's not valid.
      // remember me is in the high bits and him is in the low bits
      // so we place onto the high bits.  And d1 has the current bit mask
      if ((row | (mask << 8)) != flipt[pack_table[row]][i]) {
        push(i, y, current_depth);
        used |= mask;
        found_anything = 1;
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

    uint64_t y0 = 0;
    uint64_t y1 = load_state(used, used, used);
    y1 |= y1 << 32;

    for (int i = 1; i < 8; i++) {
      int index = 8 + y;
      int up = index + i;   // we can go off the end
      int down = index - i;

      uint64_t up_d0, up_d1, down_d0, down_d1, d;

      d = him[up];     up_d0 = load_state(d >> i, d << i, d);
      d = me[up];      up_d1 = load_state(d >> i, d << i, d);
      d = him[down]; down_d0 = load_state(d >> i, d << i, d);
      d = me[down];  down_d1 = load_state(d >> i, d << i, d);

      uint64_t d0 = (up_d0 << 32) | down_d0;
      uint64_t d1 = (up_d1 << 32) | down_d1;

      // state machine logic see above
      y0 = ((~y1) & d0) | (y0 & (y1 | d1));
      y1 |= d1 | (~d0);

      // when y1 is set the computation is finished either way, if they are all finished
      // then we can bail out.
      if ((~y1) == 0)
        break;
    }
    // read out: state 3 is valid move
    y0 &= y1;
    y0 |= y0 >> 32;
    y0 |= y0 >> 16;
    y0 |= y0 >> 8;
    row = y0 & 0xff;

    // bit_values has one bit number in each nibble
    // this saves us from looking for all 8 bits
    // when often there is only 1 bit set
    if (row) {
      uint64_t bits = bit_values[row];
      while (bits) {
        int x = (int)(bits & 0x7);
        bits >>= 4;
        push(x, y, current_depth);
        found_anything = 1;
      }
    }
  }

  return found_anything;
}

static void safe_fgets(char *s, int len, FILE *f) {
  char *result = fgets(s, len, f);
  if (!result) {
    Console.WriteLine("wrath: Unable to parse input board");
    exit(1);
  }
}

void load(const char *name, BOARD board)
{
  int i, j, n;
  char c[8];
  char s[80];

  FILE *f = fopen(name, "r");
  if (!f) {
    fprintf(stderr, "wrath: I can't open this file: '%s'\n", name);
    fflush(stderr);
    exit(1);
  }

  board = new BOARD();

  for (i = 0; i < 8; i++) {
    safe_fgets(s, sizeof(s), f);
    
    n = sscanf(s, " %c %c %c %c %c %c %c %c ", 
      &c[0], &c[1], &c[2], &c[3],
      &c[4], &c[5], &c[6], &c[7]);
    if (n != 8) {
      fprintf(stderr, "wrath: Unable to parse input board\n");
      fflush(stderr);
      exit(1);
    }

    for (j = 0; j < 8; j++)
      switch (c[j]) {

      case 'B':
      case 'b':
        board.put(false, i) = board.get(false, i) | (1 << j);
        break;

      case 'W':
      case 'w':
        board.put(true, i) = board.get(true, i) | (1 << j);
        break;

      case '-':
        break;

      default:
        fprintf(stderr, "wrath: Board has invalid characters\n");
        fflush(stderr);
        exit(1);
      }
  }

  safe_fgets(s, sizeof(s), f);
  safe_fgets(s, sizeof(s), f);
  n = sscanf(s, " %c to play", c);
  if (n != 1) {
    fprintf(stderr, "wrath: Unable to parse input board\n");
    fflush(stderr);
    exit(1);
  }
  switch (c[0]) {
  case 'w':
  case 'W':
    color = 1;
    break;
  case 'b':
  case 'B':
    color = 0;
    break;

  default:
    fprintf(stderr, "wrath: I can't tell whose turn is next\n");
    fflush(stderr);
    exit(1);
  }

  printf("Picking up where we left off... %s to play\n",
         color ? "White" : "Black");
}
#include "board.h"

}
*/