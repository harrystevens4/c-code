#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "tcp-toolkit.h"
#include "args.h"

#define PORT "8173"
#define BUFFER_MAX 1024

int server(); //server holds and sends files to clients
int client(); //receives files from the server

int main(int argc, char **argv){
	verbose_tcp_toolkit = 1;//output from tcp toolkit funcitons
	struct args args;
	parse_args(argc,argv,&args);
	printf("starting...\n");
	switch (args.single[0]){
		case 's':
			return server();
		case 'c':
			return client();
	}
	free_args(&args);
	return 0;
}

int server(){
	int client;
	int server;
	char *buffer;

	struct sockaddr client_addr;
	socklen_t client_addrlen = sizeof(client_addr);

	printf("server mode selected\n");
	server = make_server_socket(PORT, 10);//create socket, bind and start listening
	if (server < 0){
		fprintf(stderr,"ERROR: Could not start server socket.\n");
		return -1;
	}
	client = accept(server,&client_addr,&client_addrlen);
	if (client < 0){
		fprintf(stderr,"ERROR: Could not accept connection.\n");
		perror("accept");
		return -1;
	}
	printf("connection accepted.\n");
	recvall(client,&buffer);
	printf("got message %s\n",buffer);

	//cleanup
	close(client);
	return 0;
}

int client(){
	int server;

	struct sockaddr *client_addr;
	socklen_t *client_addrlen;

	printf("client mode selected\n");
	server = connect_server_socket("192.168.1.239",PORT);
	if (server < 0){
		fprintf(stderr,"ERROR: Could not connect.\n");
		return -1;
	}
	printf("Connection accepted.\n");
	
	sendall(server,"epic",5);

	//cleaing
	close(server);
	return 0;
}
