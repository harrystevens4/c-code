CC:=gcc
LFLAGS=-lpam -lpam_misc
CFLAGS=-g
pam-authenticate : pam_authenticate.o
	$(CC) $^ -o $@ $(LFLAGS)
pam_authenticate.o : ../../source-code/pam_authenticate.c
	$(CC) $(CFLAGS) -c ../../source-code/pam_authenticate.c
