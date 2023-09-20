
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"

extern char a[];
extern int colour;
extern BOARD initial;


#define INDEX(board,x,y) (!!((board)[y]&(1<<(x))))
#define RINDEX(board,x) (!!((board)&(1<<(x))))
#define TYPE(board,x,y) (INDEX((board)[0],x,y)+(INDEX((board)[1],x,y)<<1))
#define RTYPE(board,x) (RINDEX((board>>8),x)+(RINDEX((board)&0xff,x)<<1))

void safe_gets(char *buf, int len)
{
  char * result = fgets(buf, len, stdin);
  if (result) {
    // clobber the trailing \n
    buf[strlen(buf)-1] = 0;
  }
  else {
    buf[0] = 0;
  }
}

int save()
{
  register x,y;
  char name[100];
  FILE *f;

  printf("filename (press return to abort): ");
  fflush(stdout);
  safe_gets(name, sizeof(name));
  if (!name[0]) return(1);

  f=fopen(name,"w");
  if (!f) return(1);

  for (y=0;y<8;y++) {
    for (x=0;x<8;x++) { putc(a[TYPE(initial,x,y)],f); putc(' ',f);}
    fputc('\n',f);
  }
  putc('\n',f);
  fprintf(f,"%c to play\n",colour?'w':'b');
  fclose(f);
  return 0;
}

void catch()
{
  char name[100];

  printf("\nreally quit (y/n)? ");
  fflush(stdout);
  safe_gets(name, sizeof(name));
  if (name[0] != 'y') return;

  printf("save game (y/n)? ");
  fflush(stdout);
  safe_gets(name, sizeof(name));
  if (! name[0]) return;

  if (name[0] == 'y' && save()) return;

  exit(0);
}
