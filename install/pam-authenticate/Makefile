CC:=gcc
LFLAGS=
CFLAGS=
pam-authenticate : pam_authenticate.o
	$(CC) $(LFLAGS) $^ -o $@
pam_authenticate.o : ../../source-code/pam_authenticate.c
	$(CC) $(CFLAGS) -c ../../source-code/pam_authenticate.c
