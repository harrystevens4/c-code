#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "args.h"

#define HELP "Usage:\n	network-pipe [client] : connect to client\n	network-pipe : start in server mode\n -w : wait for server rather then throwing error if connection is not available"

int wait = 0;//wether we should wait for the server to come online or just exit if we cant connect

int client();
int server();

int main(int argc, char **argv){
	struct args args;
	parse_args(argc,argv,&args);
	if (args.number_multi > 0 && strcmp(args.multi[0], "--help") == 0){
		printf(HELP);
		goto cleanup;
	}
	cleanup:
	free_args(&args);
}

int client(){
}
