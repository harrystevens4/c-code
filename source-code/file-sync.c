#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "tcp-toolkit.h"
#include "args.h"

#define PORT "8173"
#define BUFFER_MAX 1024
#define KILL "stop running now"
#define START_HANDSHAKE "anyone there?"
#define CONFIRM "sure"
#define COMPLETE_HANDSHAKE "hi"
#define READY "im ready now"

int active = 1;
int verbose = 0;

int server(); //server holds and sends files to clients
int client(); //receives files from the server

int main(int argc, char **argv){
	verbose_tcp_toolkit = 0;//output from tcp toolkit funcitons
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
	size_t buffer_size;

	struct sockaddr_storage client_addr;
	socklen_t client_addrlen;

	printf("server mode selected\n");
	server = make_server_socket(PORT, 10);//create socket, bind and start listening
	if (server < 0){
		fprintf(stderr,"ERROR: Could not start server socket.\n");
		return -1;
	}
	client_addrlen = sizeof(client_addr);
	while (active){
		client = accept(server,(struct sockaddr *)&client_addr,&client_addrlen);
		if (client < 0){
			fprintf(stderr,"ERROR: Could not accept connection.\n");
			perror("accept");
			return -1;
		}
		if (verbose){
			printf("Connection accepted.\n");
		}
		buffer_size = recvall(client,&buffer);
		printf("(Client)> %s\n",buffer);
		if (buffer_size == strlen(START_HANDSHAKE)+1 && strncmp(buffer, START_HANDSHAKE, buffer_size) == 0){
			if (verbose){
				printf("Received confirmation of connection, completing handshake...\n",buffer);
			}
			printf("(Server)> "COMPLETE_HANDSHAKE"\n");
			sendall(client,COMPLETE_HANDSHAKE,strlen(COMPLETE_HANDSHAKE)+1);

		}else{
			fprintf(stderr,"ERROR: Received unexpected confirmation message of %s with buffer size of %d compared to expected %d.\n",buffer, buffer_size, strlen(START_HANDSHAKE)+1);
		}
		free(buffer);
		buffer_size = recvall(client,&buffer); //confirmation
		printf("(Client)> %s",buffer);
		free(buffer);
		if (verbose){
			printf("sending kill message...\n");
		}
		printf("(Server)> "KILL"\n");
		sendall(client,KILL,strlen(KILL)+1);
		recvall(client,&buffer);
		printf("(Client)> %s\n",buffer);
		free(buffer);
		active = 0;
		//printf("sending file...\n",buffer);
	}

	//cleanup
	close(client);
	return 0;
}

int client(){
	int server;
	char *buffer;
	size_t buffer_length = 0;

	struct sockaddr *client_addr;
	socklen_t *client_addrlen;

	printf("client mode selected\n");
	server = connect_server_socket("192.168.1.239",PORT);
	if (server < 0){
		fprintf(stderr,"ERROR: Could not connect.\n");
		return -1;
	}
	if (verbose){
		printf("Connection accepted, Starting handshake...\n");
	}
	
	printf("(Client)> "START_HANDSHAKE"\n");
	sendall(server,START_HANDSHAKE,strlen(START_HANDSHAKE)+1);

	buffer_length = recvall(server,&buffer);
	printf("(Server)> %s\n",buffer);
	if (buffer_length == strlen(COMPLETE_HANDSHAKE)+1 && strncmp(COMPLETE_HANDSHAKE, buffer, buffer_length) == 0){
		if (verbose){
			printf("Handshake completed succesfully.\n");
		}
	}else{
		close(server);
		fprintf(stderr,"ERROR: Handshake completion %s did not match %s or length %d did not match %d\n", buffer, COMPLETE_HANDSHAKE, buffer_length, strlen(COMPLETE_HANDSHAKE)+1);
		return -1;
	}

	printf("(Server)> "READY"\n");
	sendall(server,READY,strlen(READY)+1);

	while (active){
		buffer_length = recvall(server,&buffer);
		printf("(Server)> %s\n",buffer);
		if (buffer_length == strlen(KILL)+1 && strncmp(buffer, KILL, buffer_length) == 0){
			if (verbose){
				printf("Received kill signal, stopping.\n");
			}
			printf("(Client)> "CONFIRM"\n");
			sendall(server,CONFIRM,strlen(CONFIRM)+1);
			active = 0;
			break;
		}

		free(buffer);//allocated by recvall
	}

	//cleaing
	close(server);
	return 0;
}
