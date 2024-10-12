udp-broadcast : udp-broadcast.o
	gcc udp-broadcast.o -lm -o udp-broadcast
udp-broadcast.o : ../../source-code/udp-broadcast.c
	gcc -c ../../source-code/udp-broadcast.c
install : udp-broadcast
	@./install
