#include <stdio.h>
#include <math.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#define ERROR() fprintf(stderr,"--- ERROR ---\n")
#define IN_MAIN() fprintf(stderr,"[main()]\n")
#define PAYLOAD_BUFFER_SIZE 5
#define VERB if(verbose)

typedef struct payload {
	int64_t packet_number;
	int64_t total_packets;
	int32_t message_length;
	char message[PAYLOAD_BUFFER_SIZE];
} PAYLOAD;
int verbose = 0;

int broadcast_payload(char *buffer, const char port[20]);
int listen_for_payload(char **buffer, const char port[20]);
void display_help();

int main(int argc, char **argv){

	//grab the port from the argument vector
	char port[20];
	if (argc > 1 && argc < 4){
		snprintf(port,20,"%s",argv[1]);
	}else{
		//user is incompetent
		display_help();
		goto cleanup;
	}

	// decisions... decisions.
	if (argc > 2){
		//char buffer[] = "this is my buffer payload for transmition, pretty cool, eh?";
		VERB printf("transmiting payload...\n");
		broadcast_payload(argv[2],port);
	}else{
		VERB printf("listening...\n");
		char *buffer;
		int payload_len = listen_for_payload(&buffer,port);
		if (payload_len > 0){
			printf("%s\n",buffer);
		}else{
			VERB printf("could not get data.\n");
			return 1;
		}
	}
	VERB printf("done\n");
	cleanup:
	return 0;
}
int listen_for_payload(char **buffer, const char port[20]){
	int status;
	int socketfd;

	struct addrinfo hints, *info;
	memset(&hints,0,sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	//get address info
	status = getaddrinfo(NULL,port,&hints,&info);
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
	uint8_t got_all_packets = 0;
	uint8_t buffer_allocated = 0;
	int count = 0;
	int total_packets = 0;
	do{
		PAYLOAD payload;
		status = recvfrom(socketfd, &payload,sizeof(PAYLOAD),0,info->ai_addr,&(info->ai_addrlen));
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
int broadcast_payload(char *buffer, const char port[20]){
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
	status = getaddrinfo("255.255.255.255",port,&hints,&info);
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

	//split up buffer into multiple packets
	int64_t packet_count = ceil((strlen(buffer)+1)/sizeof(PAYLOAD_BUFFER_SIZE));
	for (int i = 0; i < packet_count; i++){
		PAYLOAD payload;
		int32_t message_length = strlen(buffer)+1;
		if (message_length > PAYLOAD_BUFFER_SIZE){
			message_length = PAYLOAD_BUFFER_SIZE;
		}
		memset(&payload,0,sizeof(PAYLOAD));
		payload.total_packets = packet_count;
		payload.packet_number = i;
		payload.message_length = message_length;
		strncpy(payload.message,buffer,PAYLOAD_BUFFER_SIZE);
		ssize_t buffer_size_sent = sendto(socketfd,(void *)&payload,sizeof(PAYLOAD),0,info->ai_addr,info->ai_addrlen);
		if (buffer_size_sent < 0){
			ERROR(); IN_MAIN();
			perror("sendto");
			status = -1;
			goto cleanup;
		}
		VERB printf("sent %ld bytes, message: %s\n",buffer_size_sent,payload.message);
		buffer += PAYLOAD_BUFFER_SIZE;
	}

	/* free any memory before the program ends */
	cleanup:
	freeaddrinfo(info);
	close(socketfd);
	return status;
}
void display_help(){
	printf("usage:\
	udp-broadcast <port> [message]\
	not specifying a message will result in the program listening for broadcasts\
	\n");
}
