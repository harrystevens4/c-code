CFLAGS=-Wextra -Wall -g
LFLAGS=#-fsanitize=address
main : ../../source-code/huffman.o main.o
	$(CC) -o $@ $^ $(LFLAGS)
