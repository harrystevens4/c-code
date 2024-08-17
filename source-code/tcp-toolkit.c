#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

int verbose_tcp_toolkit;

int make_socket(const char * host ,const char * port, struct addrinfo **res){
	int socketfd;

	//set up info for socket
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_socket]: Setting up socket info...\n");
	}
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));//set to zero

	//fill in hints for getaddrinfo()
	hints.ai_flags = AI_PASSIVE; //fill in our ip
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM; //byte order matters

	int result = getaddrinfo(host, port, &hints, res); //fill in addrinfo struct 'res'
	if (result != 0){
		fprintf(stderr,"ERROR [tcp-toolkit/make_socket]: getaddrinfo: %s",gai_strerror(result));
		return -1;
	}

	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_socket]: Setting up socket info done.\n");
	}

	//creating the socket
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_socket]: Creating socket...\n");
	}
	socketfd = socket((*res)->ai_family, (*res)->ai_socktype, (*res)->ai_protocol);
	if (socketfd < 0){
		fprintf(stderr,"ERROR [tcp-toolkit/make_socket]: could not create socket\n");
		perror("socket");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_socket]: Socket created with fd %d.\n",socketfd);
	}

	//return the socket file descriptor
	return socketfd;
}
int make_server_socket(const char * port, int backlog){
	struct addrinfo *res;
	int socket = make_socket(NULL, port, &res);
	if (socket < 0){
		fprintf(stderr, "ERROR: could not make the socket\n");
		return -1;
	}

	//binding the socket
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_server_socket]: Binding socket...\n");
	}
	if (bind(socket, (struct sockaddr *)res->ai_addr, res->ai_addrlen) < 0){
		fprintf(stderr,"ERROR [tpc-toolkit/make_server_socket]: could not bind socket\n");
		perror("bind");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_server_socket]: Socket bound.\n");
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_server_socket]: Starting listener.\n");
	}
	if (listen(socket,backlog) < 0){
		fprintf(stderr,"ERROR [tcp-toolkit/make_server_socket]: Could not listen.\n");
		perror("listen");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_server_socket]: Listening...\n");
	}

	//cheeky bit of cleanup
	freeaddrinfo(res);
	return socket;
}
int connect_server_socket(char * host, char * port){
	struct addrinfo *res;
	int socket = make_socket(NULL, port, &res);
	if (socket < 0){
		fprintf(stderr, "ERROR [tcp-toolkit/connect_server_socket]: could not make the socket\n");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Attempting to connect...\n");
	}
	if (connect(socket,res->ai_addr, res->ai_addrlen) < 0){
		fprintf(stderr,"ERROR [tcp-toolkit/connect_server_socket]: Could not connect.\n");
		perror("connect");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Connection successful\n");
	}

	return socket;
}
