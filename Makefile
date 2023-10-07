SOURCE = main.c display.c valid.c stack.c search.c \
    flip.c score.c tables.c fe.c comp.c user.c sort.c load.c save.c

CFLAGS = -O3

all: $(OBJECTS)
	$(CC) -o wrath $(CFLAGS) $(SOURCE)
