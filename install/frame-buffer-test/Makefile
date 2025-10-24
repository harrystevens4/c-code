CFLAGS=-g -Wall -Wextra
LIBS=#-fsanitize=address
frame-buffer-test : ../../source-code/framebuffer.o test.o
	$(CC) -o $@ $^ $(LIBS)
