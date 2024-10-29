#include <stdio.h>
#include <errno.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>

#define SEARCH_PORT "7396"
#define TRANSFER_PORT "7397"
#define CHUNK_SIZE 512

struct advertisement {
	char hostname[256];
	char filename[256];
};
struct file_chunk {
	uint8_t done;
	uint32_t data_size;
	char data[CHUNK_SIZE];
};

volatile int continue_advertising = 0;

void print_help();
int sender_main(char *);
int receiver_main();
void *advertising_agent(void* args);
int send_file(int sock, char *filename);
int recv_file(int sock, char *filename);

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
	int result = pthread_create(&agent_thread,NULL,advertising_agent,(void *)basename(filename));
	if (result < 0){
		perror("pthread_create");
		return 1;
	}

	//===================== start a server socket ================
	struct addrinfo hints, *address_info;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	result = getaddrinfo(NULL,TRANSFER_PORT,&hints,&address_info);
	if (result < 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(result));
		return 1;
	}
	int server = socket(address_info->ai_family,address_info->ai_socktype,0);
	result = bind(server,address_info->ai_addr,address_info->ai_addrlen);
	if (result < 0){
		perror("bind");
		close(server);
		return 1;
	}
	result = listen(server,1);
	if (result < 0){
		perror("listen");
		close(server);
		return 1;
	}
	int client = accept(server,address_info->ai_addr,&address_info->ai_addrlen);
	if (client < 0){
		perror("accept");
		return 1;
	}
	printf("client found.\n");
	continue_advertising = 0;
	pthread_join(agent_thread,NULL);
	
	//====== transfer the file =======
	result = send_file(client,filename);
	if (result != 0){
		printf("couldnt send file.\n");
		return result;
	}
	printf("file sent.\n");
	close(client);

	return 0;
}
int receiver_main(){
	//======== setup a broadcast listening socket ================
	struct addrinfo hints, *address_info;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result = getaddrinfo("255.255.255.255",SEARCH_PORT,&hints,&address_info);
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

	struct advertisement data;
	//========= listen for available connections ========
	for (;;){
		int result = recvfrom(broadcast_fd,&data,sizeof(struct advertisement),0, address_info->ai_addr, &address_info->ai_addrlen);
		if (result < 0){
			perror("recvfrom");
			close(broadcast_fd);
			return 1;
		}
		printf("sender %s found, with file %s. keep listening for another? (y/n)\n",data.hostname,data.filename);
		if (fgetc(stdin) == 'n') break;
	}
	freeaddrinfo(address_info);
	close(broadcast_fd);

	//======== connect to the tcp socket ===============
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	result = getaddrinfo(data.hostname,TRANSFER_PORT,&hints,&address_info);
	if (result < 0){
		fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(result));
		return 1;
	}

	int server = socket(address_info->ai_family,address_info->ai_socktype,0);
	if (socket < 0){
		perror("socket");
		return 1;
	}
	result = connect(server, address_info->ai_addr, address_info->ai_addrlen);
	if (result < 0){
		perror("connect");
		close(server);
	}
	result = recv_file(server,data.filename);
	if (result != 0){
		printf("could not get file.\n");
	}
	printf("file received.\n");
	close(server);
	return 0;
}
void *advertising_agent(void *args){
	char *filename = (char *)args;
	printf("started advertising agent.\n");
	//============= build a sock_dgram to broadcast on =========
	struct addrinfo hints, *address_info;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int result = getaddrinfo("255.255.255.255",SEARCH_PORT,&hints,&address_info);
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
		char hostname[256];
		result = gethostname(hostname, sizeof(hostname));
		if (result != 0){
			perror("gethostname");
			close(broadcast_fd);
			return (void *)1;
		}

		snprintf(data.hostname,256,"%s",hostname);
		snprintf(data.filename,256,"%s",filename);
		result = sendto(broadcast_fd,&data,sizeof(struct advertisement),0,address_info->ai_addr,address_info->ai_addrlen);
		if (result < 0){
			perror("sendto");
			close(broadcast_fd);
			return (void *)1;
		}
		sleep(1);
	}
	close(broadcast_fd);
	freeaddrinfo(address_info);
	printf("agent terminated.\n");
	
}
int send_file(int sock, char *filename){
	FILE *fp = fopen(filename,"r");
	if (fp == NULL){
		perror("fopen");
		return 1;
	}
	for (int chunk_number = 0;;chunk_number++){
		struct file_chunk buffer;
		buffer.done = 0;
		if (feof(fp)){
			break;
		}
		int result = fread(buffer.data,1,CHUNK_SIZE,fp);
		if (result < 0){
			perror("fread");
			fclose(fp);
			return 1;
		}
		buffer.data_size = result;

		result = send(sock,&buffer,sizeof(struct file_chunk),0);
		printf("sent %d bytes\n",result);
		if (result < 0){
			printf("error %d\n",errno);
			perror("write");
			fclose(fp);
			return 1;
		}
	}
	struct file_chunk buffer;
	buffer.done = 1;
	int result = write(sock,&buffer,sizeof(struct file_chunk));
	if (result < sizeof(struct file_chunk)){
		printf("should have written %ld but instead only wrote %d bytes",sizeof(struct file_chunk),result);
	}
	if (result < 0){
		perror("write");
		fclose(fp);
		return 1;
	}

	fclose(fp);
	return 0;
}
int recv_file(int sock, char *filename){
	FILE *fp = fopen(filename,"w");
	if (fp == NULL){
		return 1;
	}
	for (;;){
		struct file_chunk buffer;
		memset(&buffer,0,sizeof(struct file_chunk));
		buffer.done = 1; //safety guard
		int result = recv(sock,&buffer,sizeof(struct file_chunk),0);
		if (result < 0){
			perror("read");
			fclose(fp);
			return 1;
		}
		//printf("buffer {done:%u,data_size:%u,data:%s}\n",buffer.done,buffer.chunk_number,buffer.data_size,buffer.data);
		if (buffer.done == 1){
			printf("transmition over\n");
			break;
		}
		if (buffer.done != 1 && buffer.done != 0){
			printf("failure in transmition: buffer.done at %d\n",buffer.done);
			break;
		}
		printf("writing %d bytes\n",buffer.data_size);
		result = fwrite(buffer.data,1,buffer.data_size,fp);
		if (result < 0){
			perror("fwrite");
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}
