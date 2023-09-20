#define S slist[lvl]

static struct {
  char top;
  char x[64];
  char y[64];
  int  s[64];
} slist[2];

newlist(lvl)
{
  S.top = 0;
}

insert(x,y,score,lvl)
{
  register int i,j;

  for (i=0;i<S.top;i++)
    if (score > S.s[i]) break;

  for (j=S.top;j>i;j--) {
    S.s[j] = S.s[j-1];
    S.x[j] = S.x[j-1];
    S.y[j] = S.y[j-1];
  }

  S.s[i] = score;
  S.x[i] = x;
  S.y[i] = y;
  S.top++;
}

recmove(x,y,lvl)
int *x,*y;
{
  register int i;

  if (!S.top) {
    *x = *y = -1;
    return(0);
  }
  *x = S.x[0];
  *y = S.y[0];
  for (i=1;i<S.top;i++) {
    S.s[i-1] = S.s[i];
    S.x[i-1] = S.x[i];
    S.y[i-1] = S.y[i];
  }
  S.top--;

  return(1);
}
