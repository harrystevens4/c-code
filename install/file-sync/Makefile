file-sync : file-sync.o tcp-toolkit.o args.o
	gcc file-sync.o tcp-toolkit.o args.o -o file-sync
file-sync.o : ../../source-code/file-sync.c ../../source-code/tcp-toolkit.h ../../source-code/args.h
	gcc -c ../../source-code/file-sync.c
tcp-toolkit.o : ../../source-code/tcp-toolkit.c
	gcc -c ../../source-code/tcp-toolkit.c
args.o : ../../source-code/args.c
	gcc -c ../../source-code/args.c
