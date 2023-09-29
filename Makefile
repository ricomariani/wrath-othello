SOURCE = main.c display.c valid64.c stack.c search.c \
    flip.c score.c edge.c \
    fe.c pack.c comp.c user.c sort.c init.c save.c
     
CFLAGS = -O3 -g -Wno-implicit-function-declaration -Wno-implicit-int -Wno-unused-result

CC = clang

all: $(OBJECTS)
	$(CC) -o wrath $(CFLAGS) $(SOURCE)
