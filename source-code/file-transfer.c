#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>

#define PORT "7396"

struct advertisement {
	char ip[16];
	char filename[255];
};

volatile int continue_advertising = 0;

void print_help();
int sender_main(char *);
int receiver_main();
void *advertising_agent(void* args);

int main(int argc, char **argv){
	if (argc > 3){
		print_help();
	}
	if (argc == 2){
		sender_main(argv[1]);
	}
	if (argc == 1){
		receiver_main();
	}
	//if we are here something has gone wrong
	return EXIT_FAILURE;
}
void print_help(){
	printf("Usage: \n");
}
int sender_main(char *filename){
	//================= prep to advertise to network ===================
	//start broadcasting agent
	continue_advertising = 1;
	pthread_t agent_thread;
	int result = pthread_create(&agent_thread,NULL,advertising_agent,NULL);
	if (result < 0){
		perror("pthread_create");
		return 1;
	}

	sleep(5);
	continue_advertising = 0;
	pthread_join(agent_thread,NULL);
	return 0;

}
int receiver_main(){
	//======== setup a broadcast listening socket ================
	struct addrinfo hints, *address_info;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result = getaddrinfo("255.255.255.255",PORT,&hints,&address_info);
	if (result < 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(result));
		return 1;
	}

	int broadcast_fd = socket(address_info->ai_family,address_info->ai_socktype,0);
	if (socket < 0){
		perror("socket");
		return 1;
	}
	
	result = bind(broadcast_fd, address_info->ai_addr,address_info->ai_addrlen);
	if (result < 0){
		perror("bind");
		return 1;
	}

	//========= listen for available connections ========
	for (;;){
		struct advertisement data;
		int result = recvfrom(broadcast_fd,&data,sizeof(struct advertisement),0, address_info->ai_addr, &address_info->ai_addrlen);
		if (result < 0){
			perror("recvfrom");
			close(broadcast_fd);
			return 1;
		}
		printf("sender found at %s, with file %s. keep listening? (y/n)\n",data.ip,data.filename);
		if (fgetc(stdin) == 'n') break;
	}
	//======== connect to the tcp socket ===============
	return 0;
}
void *advertising_agent(void *args){
	printf("started advertising agent.\n");
	//============= build a sock_dgram to broadcast on =========
	struct addrinfo hints, *address_info;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result = getaddrinfo("255.255.255.255",PORT,&hints,&address_info);
	if (result < 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(result));
		return (void *)1;
	}

	int broadcast_fd = socket(address_info->ai_family,address_info->ai_socktype,0);
	if (socket < 0){
		perror("socket");
		return (void *)1;
	}

	int optval = 1;
	result = setsockopt(broadcast_fd, SOL_SOCKET,SO_BROADCAST, &optval,sizeof(optval));
	if (result != 0){
		perror("setsockopt");
		return (void *)1;
	}
	//=============== start broadcasting loop ============
	printf("ready to broadcast\n");
	for (;continue_advertising == 1;){
		struct advertisement data;
		snprintf(data.ip,16,"epic ip");
		snprintf(data.filename,255,"filename");
		result = sendto(broadcast_fd,&data,sizeof(struct advertisement),0,address_info->ai_addr,address_info->ai_addrlen);
		if (result < 0){
			perror("sendto");
			close(broadcast_fd);
			return (void *)1;
		}
		sleep(1);
	}
	close(broadcast_fd);
	printf("agent terminated.\n");
}
