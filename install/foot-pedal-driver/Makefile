CC=gcc
LFLAGS=-lX11
CFLAGS=-g
install : foot-pedal-driver
	./install
foot-pedal-driver : foot-pedal-driver.o
	$(CC) $^ -o $@ $(LFLAGS)
foot-pedal-driver.o : ../../source-code/foot-pedal-driver.c
	$(CC) -c $(CFLAGS) ../../source-code/foot-pedal-driver.c
