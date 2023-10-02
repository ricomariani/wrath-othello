SOURCE = main.c display.c valid.c stack.c search.c \
    flip.c score.c edge.c \
    fe.c pack.c comp.c user.c sort.c init.c save.c
     
CFLAGS = -O3

all: $(OBJECTS)
	$(CC) -o wrath $(CFLAGS) $(SOURCE)
