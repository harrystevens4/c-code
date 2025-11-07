CFLAGS=-g -Wall -Wextra -O3
LIBS=-fsanitize=address -O4
frame-buffer-test : ../../source-code/framebuffer.o test.o
	$(CC) -o $@ $^ $(LIBS)
