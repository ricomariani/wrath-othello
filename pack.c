#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

// The black and white occupied slots of a board row
// are represented by two bytes of a short. But there
// are only 3 combinations, none, black, and white.
// This means you can represent a given board row as
// a base three number.  pack_table[i] is the pre-computed
// number representing the row as computed by pack_board_row.
// So literally pack_table[mask] == pack_board_row(mask)
// for all valid masks.  We skip any that have both black
// and white set.
unsigned short *pack_table;


static int pack_board_row(int board_row)
{
  int s = 0;

  for (int mask = 0x01; mask <= 0x80; mask <<= 1) {
    // s += (s <<1 ) multiplies s by 3 by adding it to twice itself
    // then either add 1 or 2 depending on which bit is set
    s += (s << 1) + (!!(board_row & mask)) + (((!!(board_row & (mask << 8)))) << 1);
  }

  return s;
}

// This is only used for testing... see the test code below
static int unpack_board_row(int packed_value)
{
  int d;
  int board_row = 0;

  for (int mask = 0x80; mask; mask >>= 1) {
    // pull out the first base 3 digit
    // note that the board high bits are stored in the lowest digit
    d = packed_value % 3;

    // set up to pull out the next digit
    packed_value /= 3;

    // if the digit is zero set nothing
    if (d) {
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

void build_pack_table()
{
  pack_table = (unsigned short *)calloc(65536, sizeof(short));

  if (!pack_table) {
    printf("Error, can't allocate enough memory for pack table\n");
    exit(1);
  }
  
  printf("Building pack_board_row table\n");
  fflush(stdout);

  for (int i = 0; i < 65536; i++) {
    if (!(i & (i >> 8))) {

    // test packing/unpacking 
    // this used to be commented out because it was too slow...
    // but that was in 1988... it's fast enough now =P
    if (i != unpack_board_row(pack_board_row(i))) {
      printf("Yipe! ");
      printf("%04x %d %04x ", i, pack_board_row(i), unpack_board_row(pack_board_row(i)));
      display_one_row(i);
      printf(" != ");
      display_one_row(unpack_board_row(pack_board_row(i)));
      printf("\n");
      fflush(stdout);
      exit(99);
    }

    // any vaue where any high byte has a
    // a common bit with a low byte can be
    // skipped. It isn't a valid board row.
      pack_table[i] = pack_board_row(i);
    }
  }
}

