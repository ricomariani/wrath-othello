#include "board.h"
#include <stdio.h>
#include <stdlib.h>

BOARD initial = {
  { 0,0,0,16,8,0,0,0 },
  { 0,0,0,8,16,0,0,0 }
};

char val[256];
char val2[256];
int turn;
int consecutive_passes;
int colour = 1;
void catch();

main(argc,argv)
char **argv;
{
  int i,j;
  int player,play_side;
  FILE *f;

  player = 0;
  play_side = 0;
  if (argc >= 2) {
    switch (argv[1][0]) {

    case 'w':
    case 'W':
      player = 1;
      printf("You will be playing white.\n");
      fflush(stdout);
      play_side = 1;
      break;

    case 'b':
    case 'B':
      player = 1;
      printf("You will be playing black.\n");
      fflush(stdout);
      play_side = 0;
      break;
    }

    switch (argv[1][1]) {
    case 'l':
    case 'L':
      if (argc < 3) {
        fprintf(stderr,
           "wrath: You must specify a file name\n");
        fflush(stderr);
        exit(1);
      }
      f = fopen(argv[2],"r");
      if (!f) {
        fprintf(stderr,
           "wrath: I can't open this file: '%s'\n",
            argv[2]);
        fflush(stderr);
        exit(1);
      }
      load(f,initial);
    }
  }

  printf("Give me about thirty seconds to build up my tables please\n");
  fflush(stdout);

  bpack();
  for (i=0;i<256;i++)
    for (j=0;j<8;j++) {
      val[i] += !!(i&(1<<j));
      if (j>1 && j<6) val2[i] += !!(i&(1<<j));
      if (j==1||j==6) val2[i] -= !!(i&(1<<j));
    }
  buildedge();
  consecutive_passes = 0;

  display(initial);
  display_score(initial);
  while (consecutive_passes<2) {
    if (player && colour==play_side)
      user_input(initial,colour);
    else
      computer_input(initial,colour);

    display_score(initial);
    colour = !colour;
  }
}
