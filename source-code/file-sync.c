#include <stdio.h>
#include <sys/socket.h>
#include "tcp-toolkit.h"
#include "args.h"

#define PORT "1173"

int server(); //server holds and sends files to clients

int main(int argc, char **argv){
	verbose_tcp_toolkit = 1;//output from tcp toolkit funcitons
	struct args args;
	parse_args(argc,argv,&args);
	printf("starting...\n");
	switch (args.single[0]){
		case ('s'):
			return server();
	}
	free_args(&args);
	return 0;
}

int server(){
	int client;
	int server;

	printf("server mode selected\n");
	server = make_server_socket(PORT, 10);//create socket, bind and start listening
	//client = accept();
}
