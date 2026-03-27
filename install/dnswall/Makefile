LFLAGS=#-fsanitize=address
CFLAGS=-g
dnswall : src/main.o
	$(CC) -o $@ $^ $(LFLAGS)
