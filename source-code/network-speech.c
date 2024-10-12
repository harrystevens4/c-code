#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//socket
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

//externals
#include "../lib/external/speechd/libspeechd.h"

//speechd
#define SOCKET_PATH "speech-dispatcher/speechd.sock"
//network
#define PORT "9000"
#define PAYLOAD_BUFFER_SIZE 5

typedef struct payload {
        int64_t packet_number;
        int64_t total_packets;
        int32_t message_length;
        char message[PAYLOAD_BUFFER_SIZE];
} PAYLOAD;

int start_listening_socket(const char *port, struct addrinfo **info);
int recv_broadcast(int socketfd, char **buffer, struct addrinfo *info);
void free_recv_broadcast();

int main(int argc, char**argv){
	//get user
	char *user = getenv("USER");
	if (user == NULL){
		fprintf(stderr,"FATAL: Could not get user.\n");
		return 1;
	}

	//start up a dgram socket
	struct addrinfo *address;
	int socket_fd = start_listening_socket(PORT,&address);
	if (socket_fd < 0){
		fprintf(stderr,"FATAL: Could not start a network socket.\n");
		return 1;
	}
	//listen for a message
	char *buffer;
	recv_broadcast(socket_fd,&buffer,address);
	printf("%s\n",buffer);
	free(buffer);
	freeaddrinfo(address);

	//connect to the speech dispatcher
	SPDConnection *dispatcher;
	//                    client_name        connection_name user
	dispatcher = spd_open("dispatch-speech", "main",         (const char *)user,SPD_MODE_SINGLE);

	//send a message
	SPDPriority priority = SPD_TEXT;
	spd_say(dispatcher,priority,"");

	//cleanup
	spd_close(dispatcher);
	return 0;
}

int start_listening_socket(const char *port, struct addrinfo **address/*getaddrinfo allocates this*/){
	struct addrinfo hints;
	memset(&hints,0,sizeof(hints));
	
	//set up a udp socket
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	int result = getaddrinfo(NULL,port,&hints,address);
	if (result != 0){
		fprintf(stderr,"ERROR: getaddrinfo: %s\n",gai_strerror(result));
		return -1;
	}
	
	//create socket
	int socket_fd = socket((*address)->ai_family,(*address)->ai_socktype,(*address)->ai_protocol);
	if (socket_fd < 0){
		fprintf(stderr,"Could not create socket.\n");
		perror("socket");
		return -1;
	}

	//bind
	result = bind(socket_fd,(*address)->ai_addr,(*address)->ai_addrlen);
	if (result < 0){
		fprintf(stderr,"Could not bind the socket.\n");
		perror("bind");
		return -1;
		close(socket_fd);
	}
	//give the completed socket
	return socket_fd;

}
int recv_broadcast(int socketfd, char **buffer, struct addrinfo *info){
	int status = 0;
	uint8_t got_all_packets = 0;
	uint8_t buffer_allocated = 0;
	int count = 0;
	int total_packets = 0;
	do{
		PAYLOAD payload;
		status = recvfrom(socketfd, &payload,sizeof(PAYLOAD),0,info->ai_addr,&(info->ai_addrlen));
		if (status < 0){
			perror("recvfrom");
		}
		if (!buffer_allocated){
			buffer_allocated = 1;
			total_packets = payload.total_packets;
			*buffer = malloc(sizeof(char)*(payload.total_packets)*PAYLOAD_BUFFER_SIZE);
		}
		strncpy(*buffer+(payload.packet_number*PAYLOAD_BUFFER_SIZE),payload.message,payload.message_length);
		count += 1;
		if (count >= total_packets){
			got_all_packets = 1;
		}
	}while(!got_all_packets);
	return status;
}
