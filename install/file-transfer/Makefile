CFLAGS=-pthread -g
file-transfer : file-transfer.o
	gcc $(CFLAGS) file-transfer.o -o file-transfer
file-transfer.o : ../../source-code/file-transfer.c
	gcc $(CFLAGS) -c ../../source-code/file-transfer.c
