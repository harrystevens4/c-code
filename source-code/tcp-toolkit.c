#include <stdio.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

int verbose_tcp_toolkit;
int silent_errors = 0;
int timeout = 0;
static FILE *error_file;

int make_socket(const char * host ,const char * port, struct addrinfo **res){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
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
		fprintf(error_file,"ERROR [tcp-toolkit/make_socket]: getaddrinfo: %s",gai_strerror(result));
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
		fprintf(error_file,"ERROR [tcp-toolkit/make_socket]: could not create socket\n");
		perror("socket");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_socket]: Socket created with fd %d.\n",socketfd);
	}

	//make it reusable instantly
	int option_value = 1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &option_value, sizeof(option_value)) != 0){
		fprintf(error_file,"ERROR [tcp-toolkit/make_socket]: Could not set socket options.\n");
		perror("setsockopt");
		return -1;
	}

	//return the socket file descriptor
	return socketfd;
}
int make_server_socket(const char * port, int backlog){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	struct addrinfo *res;
	int socket = make_socket(NULL, port, &res);
	if (socket < 0){
		fprintf(error_file, "ERROR: could not make the socket\n");
		return -1;
	}

	//binding the socket
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/make_server_socket]: Binding socket...\n");
	}
	if (bind(socket, (struct sockaddr *)res->ai_addr, res->ai_addrlen) < 0){
		fprintf(error_file,"ERROR [tpc-toolkit/make_server_socket]: could not bind socket\n");
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
		fprintf(error_file,"ERROR [tcp-toolkit/make_server_socket]: Could not listen.\n");
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
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	char server_ip_address[1024];
	void *addr;
	struct addrinfo *p;
	struct addrinfo *res;
	int socket = make_socket(host, port, &res);
	if (socket < 0){
		fprintf(error_file, "ERROR [tcp-toolkit/connect_server_socket]: could not make the socket\n");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Attempting to connect...\n");
	}
	if (connect(socket,res->ai_addr, res->ai_addrlen) < 0){//something is going wrong
		fprintf(error_file,"ERROR [tcp-toolkit/connect_server_socket]: Could not connect.\n");
		if (!silent_errors) perror("connect");
		return -1;
	}
	for (p = res; p != NULL; p = p->ai_next){
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		addr = &(ipv4->sin_addr);
		inet_ntop(p->ai_family, addr, server_ip_address, sizeof(server_ip_address));
		if (verbose_tcp_toolkit){
			printf("[tcp-toolkit/connect_server_socket]: Connected to %s\n");
		}
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/connect_server_socket]: Connection successful\n");
	}

	freeaddrinfo(res);
	return socket;
}
int sendall(int socket, char * buffer, size_t buffer_length){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	int bytes_sent = 0;
	int total_bytes_sent = 0;
	int bytes_received = 0;
	int recv_buffer_size = 1024;
	char recv_buffer[recv_buffer_size];

	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/sendall]: Sending buffer size...\n");
	}
	if (send(socket, &buffer_length, sizeof(size_t), 0) < 0){//tell receiver how many bytes to expect
		fprintf(error_file,"ERROR [tcp-toolkit/sendall]: Could not send data size.\n");
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
			fprintf(error_file,"ERROR [tcp-toolkit/sendall]: Could not send data.\n");
			perror("send");
			return -1;
		}
		//shuffle pointer on to untransmitted section
		buffer += bytes_sent;
		buffer_length -= bytes_sent;
	}while (total_bytes_sent < buffer_length);
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/sendall]: Finnished sending data, waiting on acknowledgement...\n");
	}
	bytes_received = recv(socket,recv_buffer,1024,0);
	if (bytes_received < 1){
		fprintf(error_file,"ERROR [tcp-toolkit/sendall]: Connection closed before acknowlegdement received.\n");
		perror("recv");
		return -1;
	}
	
	return 0;
}
size_t recvall(int socket,char **buffer){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	unsigned short packet_size = 1024;
	size_t buffer_size;
	char * data_buffer;
	int bytes_received = 0;
	int total_bytes_received = 0;
	int result;
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Receiving buffer size...\n");
	}
	if (recv(socket,&buffer_size,sizeof(size_t),0) < 0){
		fprintf(error_file,"ERROR [tcp-toolkit/recvall]: Could not receive buffer size.\n");
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
		if (bytes_received < 1){
			fprintf(error_file,"ERROR [tcp-toolkit/recvall]: Connection closed before transmition completed.\n");
			return 0;
		}
		if (verbose_tcp_toolkit){
			printf("[tcp-toolkit/recvall]: Got packet of size %d.\n",bytes_received);
		}
		total_bytes_received += bytes_received;
	}while (total_bytes_received < buffer_size);
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Finnished receiving data, sending confirmation...\n");
	}
	result = send(socket,"OK",3,0);
	if (result < 0){
		fprintf(error_file,"ERROR [tcp-toolkit/recvall]: Could not send confirmation.\n");
		perror("send");
		return 0;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recvall]: Confirmation sent.\n");
	}
	return buffer_size;
}
int send_file(int socket, const char * filename){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	int buffer_size;
	char buffer[1024];
	char *recv_buffer;
	size_t recv_buffer_size;
	FILE *fp;
	//check file exists
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/send_file]: Checking file access...\n");
	}
	if (access(filename,F_OK) < 0){
		fprintf(error_file,"ERROR [tcp-toolkit/send_file]: File %s cannot be accessed.\n", filename);
		perror("access");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/send_file]: File access ok.\n");
	}
	fp = fopen(filename,"r");
	if (fp == NULL){
		fprintf(error_file,"ERROR [tcp-toolkit/send_file]: Could not open %s for reading.\n",filename);
		return -1;
	}
	while (1){//mainloop for transmitting file
		buffer_size = fread(buffer,1,1024,fp);
		if (buffer_size > 0){
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: Pulled char %c from file, informing client to continue receiving...\n",buffer[0]);
			}
			if (sendall(socket,"+",2) < 0){// tell the receiver to accept more packets
				fprintf(error_file,"ERROR [tcp-toolkit/send_file]: Connection closed.\n");
				return -1;
			}
			recv_buffer_size = recvall(socket,&recv_buffer);//confirmation
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: Got confirmation of %s.\n",recv_buffer);
			}
			if (recv_buffer_size < 1){
				fprintf(error_file,"[tcp-toolkit/send_file]: Connection closed.\n");
				return -1;
			}
			free(recv_buffer);
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: sending buffer now...\n");
			}
			if (sendall(socket,buffer,buffer_size) < 0){//one char per transmition
				fprintf(error_file,"ERROR [tcp-toolkit/send_file]: Connection closed.\n");
				return -1;
			}
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: sent. waiting for confirmation.\n");
			}
			recv_buffer_size = recvall(socket, &recv_buffer);
			if (recv_buffer_size < 1){
				fprintf(error_file,"ERROR [tcp-toolkit/send_file]: Client disconnected.\n");
			}
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: got confirmation %s.\n",recv_buffer);
			}
		}else{ //------------------------ dealing with the end of file -----
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/send_file]: End of file, informing client...\n");
			}
			if (sendall(socket,"done",5) < 0){// no more transmitions from us
				fprintf(error_file,"ERROR [tcp-toolkit/send_file]: Connection closed.\n");
				return -1;
			}
			break; 
		}
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/send_file]: File transmition successfull.\n");
	}
	return 0;

}
int recv_file(int socket, const char * filename){
	if (silent_errors){
		error_file = fopen("/dev/null","w");
	}else{
		error_file = stderr;
	}
	size_t buffer_length;
	char *buffer;
	int result;
	//open file for writing
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recv_file]: Opening %s for writing...\n",filename);
	}
	FILE *fp;
	fp = fopen(filename,"w");
	if (fp == NULL){
		fprintf(error_file, "[tcp-toolkit/recv_file]: Could not open %s for writing.\n",filename);
		perror("fopen");
		return -1;
	}
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recv_file]: Opened file.\n");
	}
	while (1){
		buffer_length = recvall(socket,&buffer);
		if (buffer_length < 1){
			fprintf(error_file,"ERROR [tcp-toolkit/recv_file]: Socket not connected.\n");
			return -1;
		}
		if (strncmp(buffer,"+",2) == 0 && buffer_length == 2){//server says more data
			result = sendall(socket,"OK",3); //acknowledgement
			free(buffer);
			if (result < 0){
				fprintf(error_file,"ERROR [tcp-toolkit/recv_file]: Could not send acknowlegement.\n");
				return -1;
			}
			buffer_length = recvall(socket,&buffer);
			if (buffer_length < 1){
				fprintf(error_file,"ERROR [tcp-toolkit/recv_file]: Connection closed");
				return -1;
			}
			if (verbose_tcp_toolkit){
				printf("[tcp-toolkit/recv_file]: Got %.*s.\n",buffer_length,buffer);
			}
			if (fprintf(fp,"%.*s",buffer_length,buffer) < 0){
				fprintf(error_file,"ERROR [tcp-toolkit/recv_file]: Could not write to file");
				return -1;
			}
			//printf("%.*s",buffer_length,buffer);
			//fflush(stdout);
			free(buffer);
			//confirmation
			if (sendall(socket,"OK",3) < 0){
				fprintf(error_file,"ERROR [tcp-toolkit/recv_file]: Connection closed.\n");
				return -1;
			}
		}else{
			free(buffer);
			break;
		}
	}
	fclose(fp);//clean up
	if (verbose_tcp_toolkit){
		printf("[tcp-toolkit/recv_file]: File received successfuly.\n");
	}
	return 0;
}
#define ER "ERROR "
#define BE "[tcp-toolkit/broadcast_existence]: "
int broadcast_existence(char * port){
	int return_val = 0;
	if (verbose_tcp_toolkit) printf(BE"creating socket...\n");
	
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	struct addrinfo *info, *ip_address;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result;
	result = getaddrinfo("255.255.255.255",port,&hints,&info);
	if (result != 0){
		if (!silent_errors) fprintf(stderr,ER BE "Could not get address info. %s\n",gai_strerror(result));
		return -1;
	}

	if (verbose_tcp_toolkit) printf(BE "Getting current ip address\n");
	char hostname[1024];
	if (gethostname(hostname,1024) < 0){
		if (!silent_errors) fprintf(stderr, ER BE "Could not get hostname.\n");
		if (!silent_errors) perror("gethostname");
		return -1;
	}
	result = getaddrinfo(hostname,port,&hints,&ip_address);
	if (result != 0){
		if (!silent_errors) fprintf(stderr,ER BE "Could not get address info. %s\n",gai_strerror(result));
		return -1;
	}
	char ip_string[1024];
	void * address;
	struct sockaddr_in *ipv4 = (struct sockaddr_in *)ip_address->ai_addr;
	address = &(ipv4->sin_addr);
	inet_ntop(info->ai_family,address,ip_string,sizeof(ip_string));
	if (verbose_tcp_toolkit) printf("broadcasting using message: %s\n",ip_string);

	int socketfd = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
	if (socket < 0){
		if (!silent_errors) fprintf(stderr,ER BE "Could not create socket.\n");
		if (!silent_errors) perror("socket");
		return_val -1;
		goto cleanup;
	}

	if (verbose_tcp_toolkit) printf(BE "Binding socket...\n");
	result = bind(socketfd, info->ai_addr, info->ai_addrlen);
	if (result < 0){
		if (!silent_errors) fprintf(stderr,ER BE "Could not bind socket.\n");
		if (!silent_errors) perror("bind");
		return_val -1;
		goto cleanup;
	}

	if (verbose_tcp_toolkit) printf(BE"setting socket options...\n");
	int option_value = 1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &option_value, sizeof(option_value)) != 0){
		if (!silent_errors) fprintf(stderr,ER BE"Could not set socket reuse.\n");
		if (!silent_errors) perror("setsockopt");
		return_val = -1;
		goto cleanup;
	}
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST, &option_value, sizeof(option_value)) != 0){
		if (!silent_errors) fprintf(stderr,ER BE"Could not set socket broadcast.\n");
		if (!silent_errors) perror("setsockopt");
		return_val = -1;
		goto cleanup;
	}

	if (verbose_tcp_toolkit) printf(BE"starting broadcast.\n");
	int bytes_sent;
	bytes_sent = sendto(socketfd, ip_string, strlen(ip_string)+1, 0,info->ai_addr,info->ai_addrlen);
	if (bytes_sent < 0){
		if (!silent_errors) fprintf(stderr,ER BE "Could not send data.\n");
		if (!silent_errors) perror("sendto");
		return_val = -1;
		goto cleanup;
	}
	if (verbose_tcp_toolkit) printf(BE "broadcast complete, sending %.*s at %d bytes.\n",bytes_sent,ip_string,bytes_sent);
	
	cleanup:
	close(socketfd);
	freeaddrinfo(info);
	return return_val;
}
#define FB "[tcp-toolkit/find_broadcasters]: "
char * find_broadcasters(char * port){
	char return_val_buffer[1024];
	char *return_val = NULL;
	if (verbose_tcp_toolkit) printf(FB"creating socket...\n");
	
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	struct addrinfo *info;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result;
	result = getaddrinfo(NULL,port,&hints,&info);
	if (result != 0){
		if (!silent_errors) fprintf(stderr,ER FB "Could not get address info. %s\n",gai_strerror(result));
		return return_val;
	}

	int socketfd = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
	if (socket < 0){
		if (!silent_errors) fprintf(stderr,ER FB "Could not create socket.\n");
		if (!silent_errors) perror("socket");
		goto cleanup;
	}

	if (verbose_tcp_toolkit) printf(BE "Binding socket...\n");
	result = bind(socketfd, info->ai_addr, info->ai_addrlen);
	if (result < 0){
		if (!silent_errors) fprintf(stderr,ER FB "Could not bind socket.\n");
		if (!silent_errors) perror("bind");
		goto cleanup;
	}

	if (verbose_tcp_toolkit) printf(FB"setting socket options...\n");
	int option_value = 1;
	if(setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &option_value, sizeof(option_value)) != 0){
		if (!silent_errors) fprintf(stderr,ER FB"Could not set socket reuse.\n");
		if (!silent_errors) perror("setsockopt");
		goto cleanup;
	}
	if(setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST, &option_value, sizeof(option_value)) != 0){
		if (!silent_errors) fprintf(stderr,ER FB"Could not set socket broadcast.\n");
		if (!silent_errors) perror("setsockopt");
		goto cleanup;
	}
	if (timeout > 0){
		if (verbose_tcp_toolkit) printf(FB "Setting timeout.\n");
		struct timeval timeout_tv;
		timeout_tv.tv_sec = timeout;
		timeout_tv.tv_usec = 0;
		if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO, &timeout_tv, sizeof(timeout_tv)) < 0){
			if (!silent_errors) fprintf(stderr,ER FB "could not set timeout.\n");
			if (!silent_errors) perror("setsockopt");
			goto cleanup;
		}
		if (verbose_tcp_toolkit) printf(FB "socket timeout set.\n");
	}

	if (verbose_tcp_toolkit) printf(FB"listening...\n");
	int addrlen = info->ai_addrlen;
	int bytes_received;
	char recv_buffer[1024];
	bytes_received = recvfrom(socketfd,recv_buffer, 1024, 0,info->ai_addr,&addrlen);
	if (bytes_received < 0){
		if (!silent_errors) fprintf(stderr,ER FB "Could not receive data.\n");
		if (!silent_errors) perror("recvfrom");
		goto cleanup;
	}
	return_val = return_val_buffer;
	snprintf(return_val,1024,"%s",recv_buffer);
	if (verbose_tcp_toolkit) printf(FB "Got data of size %d\n",bytes_received);
	
	cleanup:
	close(socketfd);
	freeaddrinfo(info);
	return return_val;
}
