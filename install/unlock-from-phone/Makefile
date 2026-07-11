LIBS=
CFLAGS=-Wall -Wextra -g

remoteunlockd : src/main.o ../../source-code/base_n.o ../../source-code/crypto.o
	$(CC) -o $@ $^ $(LIBS)
