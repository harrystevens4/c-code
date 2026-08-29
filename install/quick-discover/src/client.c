#include "../../../source-code/quick-discover.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))

void _close(void *arg){
	int fd = *(int *)arg;
	close(fd);
}

void print_sockaddr(struct sockaddr *addr);

int main(int argc, char **argv){
	//====== prep discover request ======
	struct qd_discover_packet discover_packet = {0};
	//====== command line arguments ======
	if (argc < 2){
		fprintf(stderr,"Please use subcommand \"service\", \"hostname\" or \"all\"\n");
		return 1;
	}
	if (strcmp("service",argv[1]) == 0){
		if (argc < 3){
			fprintf(stderr,"Please provide a service name\n");
			return 1;
		}
		strncpy(discover_packet.service,argv[2],QD_SERVICE_LEN);
	}else if (strcmp("hostname",argv[1]) == 0){
		if (argc < 3){
			fprintf(stderr,"Please provide a hostname\n");
			return 1;
		}
		strncpy(discover_packet.hostname,argv[2],HOSTNAME_MAX_LEN);
		discover_packet.hostname_len = MIN(strlen(argv[2]),HOSTNAME_MAX_LEN);
	}else {
		fprintf(stderr,"Subcommand unrecognised\n");
		return 1;
	}
	//====== make the request ======
	__attribute__((__cleanup__(_close))) int qdfd = qd_client_socket();
	if (qdfd < 0){
		perror("qd_client_socket");
		return 1;
	}
	int result = qd_send_discover(qdfd,&discover_packet);
	if (result < 0){
		perror("qd_send_discover");
		return 1;
	}
	struct qd_response_packet response_packet = {0};
	result = qd_recv_response(qdfd,&response_packet);
	if (result < 0){
		perror("qd_recv_response");
		return 1;
	}
	//====== read response ======
	struct sockaddr_storage aligned_address = {0};
	memcpy(&aligned_address,&response_packet.address,sizeof(struct sockaddr_storage));
	printf("got response:\n");
	printf("address: ");
	print_sockaddr((struct sockaddr *)&aligned_address);
	printf("\n");
	//====== cleanup ======
	return 0;
}

void print_sockaddr(struct sockaddr *addr){
	char buffer[1024] = {0};
	if (addr->sa_family == AF_INET){
		struct sockaddr_in *addr = (struct sockaddr_in *)addr;
		inet_ntop(AF_INET,&addr->sin_addr,buffer,1024);
		printf("%s",buffer);
	}else if (addr->sa_family == AF_INET6){
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)addr;
		inet_ntop(AF_INET6,&addr->sin6_addr,buffer,1024);
		printf("%s",buffer);
	}else {
		printf("N/A");
	}
}
