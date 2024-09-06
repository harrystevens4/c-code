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

struct payload {
	int32_t message_length;
	char message[1024];
};

int main(){
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

	//prepare payload
	struct payload payload;
	memset(&payload,0,sizeof(payload));
	char message_buffer[] = "epic message";
	snprintf(payload.message,sizeof(payload.message),"%s",message_buffer);
	payload.message_length = strlen(message_buffer)+1;

	ssize_t buffer_size_sent = sendto(socketfd,&payload,sizeof(payload),0,info->ai_addr,info->ai_addrlen);
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
