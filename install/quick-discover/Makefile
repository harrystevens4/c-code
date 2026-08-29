CFLAGS=-g -Wall -Wextra
LFLAGS=

all : quick-discover quick-discover-server

quick-discover-server : src/server.o ../../source-code/quick-discover.o
	$(CC) -o $@ $^ $(LFLAGS)

quick-discover : src/client.o ../../source-code/quick-discover.o
	$(CC) -o $@ $^ $(LFLAGS)
