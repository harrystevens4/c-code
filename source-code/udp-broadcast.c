#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#define ERROR() fprintf(stderr,"--- ERROR ---\n")
#define IN_MAIN() fprintf(stderr,"[main()]\n")

typedef struct payload {
	int32_t message_length;
	char message[1024];
} PAYLOAD;
int broadcast_payload(PAYLOAD * payload);
int listen_for_payload(PAYLOAD * payload);

int main(int argc, char **argv){

	//set up the payload for transmition
	printf("preparing payload...\n");
	PAYLOAD payload;
	printf("clearing mem of size %ld...\n",sizeof(PAYLOAD));
	memset(&payload,0,sizeof(PAYLOAD));
	char message_buffer[] = "You, yes im talking to you should stop reading my packets. NOT COOL.";
	printf("inserting message buffer...\n");
	snprintf(payload.message,sizeof(payload.message),"%s",message_buffer);
	printf("inserting message length...\n");
	payload.message_length = strlen(message_buffer)+1;
	printf("done.\n");

	// decisions... decisions.
	if (argc > 1){
		printf("broadcasting...\n");
		broadcast_payload(&payload);
	}else{
		printf("listening...\n");
		int payload_len = listen_for_payload(&payload);
		if (payload_len > 0){
			printf("payload: %s\n",payload.message);
		}else{
			printf("could not get data.\n");
			return 1;
		}
	}
	printf("done\n");
	return 0;
}
int listen_for_payload(PAYLOAD * payload){
	int status;
	int socketfd;

	struct addrinfo hints, *info;
	memset(&hints,0,sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	//get address info
	status = getaddrinfo(NULL,"7000",&hints,&info);
	if (status != 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
		return -1;
	}
	
	//create the socket
	socketfd = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
	if (socketfd < 0){
		perror("socket");
		return -1;
	}
	
	//bind the socket
	status = bind(socketfd,info->ai_addr,info->ai_addrlen);
	if (status < 0){
		perror("bind");
		return -1;
	}

	//listen for the data
	status = recvfrom(socketfd, payload,sizeof(PAYLOAD),0,info->ai_addr,&(info->ai_addrlen));

	return status;
}
int broadcast_payload(PAYLOAD * payload){
	int status = 0; /* hold the return value of funcitons */
	int socketfd;

	//set up structs and such
	struct addrinfo hints, *info;
	memset(&info,0,sizeof(info));
	memset(&hints,0,sizeof(hints));
	
	//fill in hints struct to pass to getaddrinfo
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	//fill in main addrinfo struct
	status = getaddrinfo("255.255.255.255","7000",&hints,&info);
	if (status != 0){
		ERROR(); IN_MAIN();
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(status));
		goto cleanup;
	}

	//create the socket
	socketfd = socket(info->ai_family,info->ai_socktype,info->ai_protocol);
	if (socketfd < 0){
		ERROR(); IN_MAIN();
		perror("socket");
		status = -1;
		goto cleanup;
	}

	//make socket reusable instantly
        int option_value = 1;
        status = setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &option_value, sizeof(option_value));
        status += setsockopt(socketfd,SOL_SOCKET,SO_BROADCAST, &option_value, sizeof(option_value));
	if (status != 0){
		ERROR(); IN_MAIN();
                goto cleanup;
	}


	ssize_t buffer_size_sent = sendto(socketfd,(void *)payload,sizeof(PAYLOAD),0,info->ai_addr,info->ai_addrlen);
	if (buffer_size_sent < 0){
		ERROR(); IN_MAIN();
		perror("sendto");
		status = -1;
		goto cleanup;
	}
	printf("sent %ld bytes\n",buffer_size_sent);

	/* free any memory before the program ends */
	cleanup:
	freeaddrinfo(info);
	close(socketfd);
	return status;
}
