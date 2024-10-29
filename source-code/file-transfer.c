#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
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
#define CHUNK_SIZE 2048
#define MAX_CONSECUTIVE_PACKETS 2
#define PROGRESS_UPDATE_INTERVAL 20

struct advertisement {
	char hostname[256];
	char filename[256];
};
struct file_chunk {
	uint8_t done;
	uint8_t ack_required;
	uint32_t data_size;
	uint64_t filesize;
	char data[CHUNK_SIZE];
};
struct acknowledgement {
	uint8_t ready;
};

volatile int continue_advertising = 0;

void print_help();
int sender_main(char *);
int receiver_main();
void *advertising_agent(void* args);
int send_file(int sock, char *filename);
int recv_file(int sock, char *filename);
long long int get_filesize(char *filename);
void render_progress(float percent);

int main(int argc, char **argv){
	if (argc > 3){
		print_help();
		return 1;
	}
	if (argc == 2){
		return sender_main(argv[1]);
	}
	if (argc == 1){
		return receiver_main();
	}
	//if we are here something has gone wrong
	return 1;
}
void print_help(){
	printf("Usage: file-transfer [filename]\n");
	printf("Not specifying the filename will result in it listening for files.\n");
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
	if (server < 0){
		perror("socket");
		return 1;
	}
	int optval = 1;
	result = setsockopt(server, SOL_SOCKET,SO_REUSEADDR, &optval,sizeof(optval));
	if (result != 0){
		perror("setsockopt");
		return 1;
	}
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
	close(server);
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
	result = recvfrom(broadcast_fd,&data,sizeof(struct advertisement),0, address_info->ai_addr, &address_info->ai_addrlen);
	if (result < 0){
		perror("recvfrom");
		close(broadcast_fd);
		return 1;
	}
	printf("Sender %s found, with file %s. Download? (y/n)",data.hostname,data.filename);
	if (fgetc(stdin) != 'y') return 0;
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
	result = setsockopt(broadcast_fd, SOL_SOCKET,SO_BROADCAST | SO_REUSEADDR, &optval,sizeof(optval));
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
	long long int filesize = get_filesize(filename);
	if (filesize < 0){
		return 1;
	}
	printf("filesize: %lld\n",filesize);
	FILE *fp = fopen(filename,"r");
	if (fp == NULL){
		perror("fopen");
		return 1;
	}
	int packet = 0;
	for (int chunk_number = 0;;chunk_number++){
		struct file_chunk buffer;
		memset(&buffer,0,sizeof(struct file_chunk));
		if (packet >= MAX_CONSECUTIVE_PACKETS){
			buffer.ack_required = 1;
		}else{
			buffer.ack_required = 0;
		}
		buffer.done = 0;
		buffer.filesize = filesize;
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

		char *transmit_buffer = (char *)&buffer;
		int transmit_buffer_size = sizeof(struct file_chunk);
		for (;;){
			result = send(sock,transmit_buffer,transmit_buffer_size,0);
			//printf("sent %d bytes\n",result);
			if (result < 0){
				printf("error %d\n",errno);
				perror("write");
				fclose(fp);
				return 1;
			}else if (result < transmit_buffer_size){
				transmit_buffer += result;
				transmit_buffer_size -= result;
				continue;
			}else{
				break;
			}
		}
		if (chunk_number >= PROGRESS_UPDATE_INTERVAL){
			float progress_percent = (float)ftell(fp)/(float)(filesize);
			render_progress(progress_percent);
			chunk_number = 0;
		}
		if (packet >= MAX_CONSECUTIVE_PACKETS){
			struct acknowledgement ack;
			//printf("waiting for ack...\n");
			int result = recv(sock,&ack,sizeof(struct acknowledgement),0);
			if (result < 0){
				perror("recv");
				fclose(fp);
				return 1;
			}
			packet = 0;
		}
		packet ++;
	}
	printf("\n");
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
	int packet = 0;
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
			break;
		}
		if (buffer.done != 1 && buffer.done != 0){
			printf("failure in transmition: buffer.done at %d\n",buffer.done);
			break;
		}
		//printf("writing %d bytes\n",buffer.data_size);
		result = fwrite(buffer.data,1,buffer.data_size,fp);
		if (result < 0){
			perror("fwrite");
			fclose(fp);
			return 1;
		}
		
		//-------- render a percent bar every PROGRESS_UPDATE_INTERVAL ------
		if (packet >= PROGRESS_UPDATE_INTERVAL){
			float progress_percent = (float)ftell(fp)/(float)(buffer.filesize);
			packet = 0;
			render_progress(progress_percent);
		}
		packet++;
			

		if (buffer.ack_required == 1){
			//printf("sending ack\n");
			struct acknowledgement ack;
			result = send(sock,&ack,sizeof(struct acknowledgement),0);
			if (result < 0){
				perror("send");
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);
	printf("\n");
	return 0;
}
long long int get_filesize(char *filename){
	FILE *fp = fopen(filename,"r");
	if (fp == NULL){
		perror("fopen");
		return -1;
	}
	int result = fseek(fp,0,SEEK_END);
	if (result < 0){
		perror("fseek");
		return -1;
	}
	long long int size = ftell(fp);
	if (size < 0){
		perror("ftell");
	}
	fclose(fp);
	return size;
}
void render_progress(float percent){ //decimal like 0.2
	static unsigned long start_time_us = 0;
	static float last_percent;
	struct winsize term_size;
	int result = ioctl(STDOUT_FILENO, TIOCGWINSZ, &term_size);
	int progress_width = term_size.ws_col-12;
	int time_remaining_width = term_size.ws_col-progress_width;
	//======= figure out time remaining =======
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long time_us = (1000000*tv.tv_sec) + tv.tv_usec;
	if (start_time_us == 0){
		start_time_us = time_us-1;
	}
	unsigned long time_difference_us = time_us-start_time_us;
	unsigned long time_remaining = ((0-(1-(1/(percent+0.01))))*time_difference_us)/1000000; //dont divide by 0

	//======== show bar =======
	if (result < 0){
		perror("ioctl");
		return;
	}
	printf("\r[");
	int i;
	for (i = 0; i < percent*progress_width; i++){
		putchar('=');
	}
	for (; i < progress_width; i++){
		putchar(' ');
	}
	//======= show time remaining =====
	putchar(']');
	printf("%*lds",time_remaining_width-3,time_remaining);
	fflush(stdout);
	//for next call
}
