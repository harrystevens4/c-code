#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "args.h"
#include "tcp-toolkit.h"
#include <sys/socket.h>

#define HELP "Usage:\n	network-pipe [client] [port]: connect to client\n	network-pipe : start in server mode\n -w : wait for server rather then throwing error if connection is not available\n"

int wait = 0;//wether we should wait for the server to come online or just exit if we cant connect

int client(char * host, char * port);
int server(char * port);

int main(int argc, char **argv){
	verbose_tcp_toolkit = 0;
	silent_errors = 1;
	int result = 0;
	struct args args;
	parse_args(argc,argv,&args);

	//detect args
	for (int i; i < args.number_single; i++){
		if (args.single[i] == 'w') wait = 1;
	}

	//server code
	if (args.number_other == 1){
		result = server(args.other[0]);
		goto cleanup;
	}

	//client code
	if (args.number_other == 2){
		result = client(args.other[0],args.other[1]); 
		goto cleanup;
	}
	
	//help text
	printf(HELP);

	cleanup:
	free_args(&args);
	return result;
}

int client(char * host, char * port){
	int server;
	do{
		server = connect_server_socket(host,port);
		if (server < 0 && wait != 1){
			fprintf(stderr,"Could not connect to server.\n");
			return 1;
		}
		if (server > 0){
			break;
		}
		sleep(1);
	}while (wait);

	while (1)
	{
		char *buffer;
		size_t buffer_size;
		
		buffer_size = recvall(server,&buffer);
		printf("%.*s",buffer_size,buffer);
		
		if (buffer_size < 1) break;
		free(buffer);
	}
	close(server);
}
int server(char * port){
	//set up server socket to receive connections
	int server = make_server_socket(port,1);
	if (server < 0){
		fprintf(stderr,"ERROR: Could not create server socket.\n"); 
		return 1;
	}

	int client = accept(server,NULL,NULL);
	
	while (1)
	{
		size_t buffer_size;
		char buffer[1024];
		int result = -1;
		
		buffer_size = fread(buffer,1,1024,stdin);
		result = sendall(client,buffer,buffer_size);
		
		if (result < 0) break;
		if (feof(stdin)) break;
	}

	//cleanup
	int result;
	close(client);
	close(server);
}
