CFLAGS=-g -Wall -Wextra `pkg-config --cflags ncursesw`
LIBS=-lncursesw
termtime : ../../source-code/termtime.o
	$(CC) -o $@ $^ $(LIBS) 
