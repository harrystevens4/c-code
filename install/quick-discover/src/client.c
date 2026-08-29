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

int main(int argc, char **argv){
	char *hostname = NULL;
	char *service = NULL;
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
		service = argv[2];
	}else if (strcmp("hostname",argv[1]) == 0){
		if (argc < 3){
			fprintf(stderr,"Please provide a hostname\n");
			return 1;
		}
		hostname = argv[2];
	}else if (strcmp("all",argv[1]) == 0){
		;
	}else {
		fprintf(stderr,"Subcommand unrecognised\n");
		return 1;
	}
	//====== make the request ======
	struct qd_response *response = qd_discover(service,hostname,250);
	if (response == NULL){
		perror("qd_discover");
		return 1;
	}
	//====== read responses ======
	for (struct qd_response *current_response = response; current_response != NULL; current_response = current_response->next){
		printf("got response:\n");
		//address
		char address_buffer[1024] = {0};
		if (current_response->addr->sa_family){
			struct sockaddr_in *in_addr = (struct sockaddr_in *)current_response->addr;
			inet_ntop(AF_INET,&in_addr->sin_addr,address_buffer,1024);
		}else {
			struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)current_response->addr;
			inet_ntop(AF_INET6,&in6_addr->sin6_addr,address_buffer,1024);
		}
		printf("address: %s\n",address_buffer);
		//hostname
		printf("hostname: %s\n",current_response->hostname);
		//service
		printf("service: %.*s\n",QD_SERVICE_LEN,current_response->service);
	}
	//====== cleanup ======
	qd_response_free(response);
	return 0;
}
