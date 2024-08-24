network-search : network-search.o tcp-toolkit.o args.o
	gcc network-search.o tcp-toolkit.o args.o -o network-search
network-search.o : ../../source-code/network-search.c ../../source-code/tcp-toolkit.h ../../source-code/args.h
	gcc -c ../../source-code/network-search.c
args.o : ../../source-code/args.c
	gcc -c ../../source-code/args.c
tcp-toolkit.o : ../../source-code/tcp-toolkit.c
	gcc -c ../../source-code/tcp-toolkit.c
