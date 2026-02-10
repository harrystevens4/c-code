CFLAGS=-g -Wall -Wextra

test : test.o my-heap-allocator.o
	gcc -o $@ $^

libmha.so : my-heap-allocator.o
	gcc -shared -o $@ $^
