#define S stacks[lvl]

static struct {
	int top;
	char x[64];
	char y[64];
	char junk[124];
} stacks[64];

new(lvl)
{
	S.top = 0;
}

push(x,y,lvl)
{

	S.x[S.top] = x;
	S.y[S.top++] = y;
}

pop(x,y,lvl)
int *x,*y;
{

	if (!S.top) {
		*x = *y = -1;
		return(0);
	}
	*x = S.x[--S.top];
	*y = S.y[S.top];
	return(1);
}
