#include <stdio.h>
#include <arpa/inet.h>
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
	if (host == NULL){
		hints.ai_flags = AI_PASSIVE; //fill in our ip
	}
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
	char server_ip_address[1024];
	void *addr;
	struct addrinfo *p;
	struct addrinfo *res;
	int socket = make_socket(host, port, &res);
	if (socket < 0){
		fprintf(stderr, "ERROR [tcp-toolkit/connect_server_socket]: could not make the socket\n");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Attempting to connect...\n");
	}
	if (connect(socket,res->ai_addr, res->ai_addrlen) < 0){//something is going wrong
		fprintf(stderr,"ERROR [tcp-toolkit/connect_server_socket]: Could not connect.\n");
		perror("connect");
		return -1;
	}
	for (p = res; p != NULL; p = p->ai_next){
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
		inet_ntop(p->ai_family, addr, server_ip_address, sizeof(server_ip_address));
		printf("[tcp-toolkit/connect_server_socket]: Connected to %s\n");
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Connection successful\n");
	}

	freeaddrinfo(res);
	return socket;
}
int sendall(int socket, char * buffer, size_t buffer_length){
	int bytes_sent = 0;
	int total_bytes_sent = 0;

	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/sendall]: Sending buffer size...\n");
	}
	if (send(socket, &buffer_length, sizeof(size_t), 0) < 0){//tell receiver how many bytes to expect
		fprintf(stderr,"ERROR [tcp-toolkit/sendall]: Could not send data size.\n");
		perror("send");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/sendall]: Sent buffer size.\n");
	}

	do{
		bytes_sent = send(socket, buffer, buffer_length, 0);
		if (verbose_tcp_toolkit){
			printf("[tcp-toolkit/sendall]: sent %d/%d bytes\n",bytes_sent,buffer_length);
		}
		if (bytes_sent < 0){
			fprintf(stderr,"ERROR [tcp-toolkit/sendall]: Could not send data.\n");
			perror("send");
			return -1;
		}
		//shuffle pointer on to untransmitted section
		buffer += bytes_sent;
		buffer_length -= bytes_sent;
	}while (total_bytes_sent < buffer_length);
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/sendall]: Finnished sending data.\n");
	}
	return 0;
}
size_t recvall(int socket,char **buffer){
	unsigned short packet_size = 1024;
	size_t buffer_size;
	char * data_buffer;
	int bytes_received = 0;
	int total_bytes_received = 0;
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Receiving buffer size...\n");
	}
	if (recv(socket,&buffer_size,sizeof(size_t),0) < 0){
		fprintf(stderr,"ERROR [tcp-toolkit/recvall]: Could not receive buffer size.\n");
		perror("recv");
		return 0;//returning a size_t which is unsigned
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Got buffer size of %d\n",buffer_size);
	}
	*buffer = malloc(buffer_size);
	do{
		if (verbose_tcp_toolkit){
			printf("[tcp-toolkit/recvall]: Receiving packet...\n");
		}
		bytes_received = recv(socket,(*buffer)+total_bytes_received,packet_size,0);
		if (bytes_received == 0){
			fprintf(stderr,"ERROR [tcp-toolkit/recvall]: Connection closed before transmition completed.\n");
			return 0;
		}
		if (verbose_tcp_toolkit){
			printf("[tcp-toolkit/recvall]: Got packet of size %d.\n",bytes_received);
		}
		total_bytes_received += bytes_received;
	}while (total_bytes_received < buffer_size);
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Finnished receiving data.\n");
	}
	return buffer_size;
}
