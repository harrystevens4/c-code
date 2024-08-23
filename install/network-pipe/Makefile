network-pipe : network-pipe.o args.o tcp-toolkit.o
	gcc network-pipe.o args.o tcp-toolkit.o -o network-pipe
network-pipe.o : ../../source-code/network-pipe.c
	gcc -c ../../source-code/network-pipe.c
args.o : ../../source-code/args.h ../../source-code/args.c
	gcc -c ../../source-code/args.c
tcp-toolkit.o : ../../source-code/tcp-toolkit.c ../../source-code/tcp-toolkit.h
	gcc -c ../../source-code/tcp-toolkit.c
