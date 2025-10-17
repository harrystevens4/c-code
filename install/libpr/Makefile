CFLAGS=-Wall -g -Wextra -fPIC
libpr.so : ../../source-code/process-restrict.c
	$(CC) -shared -fPIC -o $@ $^
fish-restrict : fish-restrict.o
	$(CC) -o $@ $^
