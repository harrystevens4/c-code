#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/socket.h>
#include "tcp-toolkit.h"
#include "args.h"

#define USAGE "Usage:\n	-c : client mode\n	-s <config file>: server mode\n	-q : quiet mode (no output)"
#define PORT "8173"
#define BUFFER_MAX 1024
#define KILL "stop running now"
#define START_HANDSHAKE "anyone there?"
#define CONFIRM "sure"
#define COMPLETE_HANDSHAKE "hi"
#define READY "im ready now"
#define SENDING_FILE "im about to send a file"

int active = 1;
int verbose = 0;

char config_filename[PATH_MAX];

int server(); //server holds and sends files to clients
int client(); //receives files from the server

int main(int argc, char **argv){
	int mode = 0;//what the program does (server client or help)
	verbose_tcp_toolkit = 1;//output from tcp toolkit funcitons
	verbose = 1;
	struct args args;
	parse_args(argc,argv,&args);
	if (args.number_single < 1){
		printf(USAGE);
		return -1;
	}
	printf("starting...\n");
	switch (args.single[0]){
		case 's':
			if (args.number_other < 1 || access(args.other[0], F_OK) != 0){
				printf("Please specify the config file.\n"USAGE);
				return -1;
			}
			snprintf(config_filename,PATH_MAX,"%s",args.other[0]);
			mode = 1;
			break;
		case 'c':
			mode = 2;
			break;
		case 'q':
			daemon(1,0);//redirect stdin out and err to /dev/null
			break;
	}
	switch (mode){
		case 0:
			printf(USAGE);//help text
		case 1:
			return server();
		case 2:
			return client();
	}
	free_args(&args);
	return 0;
}

int server(){
	FILE *config_file;
	int client;
	int server;
	char *buffer;
	size_t buffer_size;
	int not_done = 1;
	char line_buffer[PATH_MAX];
	char char_buffer;
	int result = 0;

	struct sockaddr_storage client_addr;
	socklen_t client_addrlen;

	printf("server mode selected\n");
	server = make_server_socket(PORT, 10);//create socket, bind and start listening
	if (server < 0){
		fprintf(stderr,"ERROR: Could not start server socket.\n");
		return -1;
	}
	client_addrlen = sizeof(client_addr);
	while (active){// dealing with clients mainloop
		client = accept(server,(struct sockaddr *)&client_addr,&client_addrlen);
		if (client < 0){
			fprintf(stderr,"ERROR: Could not accept connection.\n");
			perror("accept");
			return -1;
		}
		if (verbose){
			printf("Connection accepted.\n");
		}
		buffer_size = recvall(client,&buffer);
		printf("(Client)> %s\n",buffer);
		if (buffer_size == strlen(START_HANDSHAKE)+1 && strncmp(buffer, START_HANDSHAKE, buffer_size) == 0){
			if (verbose){
				printf("Received confirmation of connection, completing handshake...\n",buffer);
			}
			printf("(Server)> "COMPLETE_HANDSHAKE"\n");
			sendall(client,COMPLETE_HANDSHAKE,strlen(COMPLETE_HANDSHAKE)+1);

		}else{
			fprintf(stderr,"ERROR: Received unexpected confirmation message of %s with buffer size of %d compared to expected %d.\n",buffer, buffer_size, strlen(START_HANDSHAKE)+1);
			close(client);
			free(buffer);
			continue;
		}
		free(buffer);
		buffer_size = recvall(client,&buffer); //confirmation
		if (buffer_size < 1){
			fprintf(stderr,"ERROR: No confirmation sent.\n");
			free(buffer);
			close(client);
			return -1;
		}
		printf("(Client)> %s\n",buffer);
		free(buffer);

		if (verbose){
			printf("reading config file...\n");
		}
		
		config_file = fopen(config_filename,"r");
		if (config_file == NULL){
			fprintf(stderr,"ERROR: Could not open config file.");
			not_done = 0;//skip reading the config
		}
		while (not_done){
			line_buffer[0] = '\0';
			for (int i = 0;;i++){//read untill newline and increment i
				int not_comment = 1;
				char_buffer = fgetc(config_file);
				if (feof(config_file)){
					not_done = 0;
					break;
				}
				if (char_buffer == '\n'){//break at newlines
					break;
				}
				if (char_buffer == '#'){
					not_comment = 0;
				}
				if (i < PATH_MAX-1 && not_comment){//check there is space
					line_buffer[i] = char_buffer;
				}
				line_buffer[i+1] = '\0';
			}
			if (line_buffer[0] == '\0'){
				continue;//dont use empty lines
			}
			if (verbose){
				printf("got line %s\n",line_buffer);
			}
			if (access(line_buffer,F_OK) == 0){
				//tell client to expect a file
				printf("(Server)> %s\n",SENDING_FILE);
				result = sendall(client,SENDING_FILE,strlen(SENDING_FILE)+1);
				if (result < 0){
					fprintf(stderr, "ERROR: Could not inform client of incoming file.\n");
					close(client);
					return -1;
				}
				buffer_size = recvall(client,&buffer);
				printf("(Client)> %s\n",buffer);
				if (buffer_size < 1){
					fprintf(stderr, "ERROR: Client disconnected.\n");
					close(client);
					return -1;
				}
				free(buffer);
				//start file transmition
				send_file(client,line_buffer);
			}else{
				fprintf(stderr,"ERROR: config file: file [%s] not found.\n",line_buffer);
			}
		}
		fclose(config_file);//clean it up

		if (verbose){
			printf("sending kill message...\n");
		}
		printf("(Server)> "KILL"\n");
		sendall(client,KILL,strlen(KILL)+1);
		recvall(client,&buffer);
		printf("(Client)> %s\n",buffer);
		free(buffer);
		close(client);
		active = 0;
		//printf("sending file...\n",buffer);
	}

	//cleanup
	return 0;
}

int client(){
	int result = 0;
	int server;
	char *buffer;
	size_t buffer_length = 0;

	struct sockaddr *client_addr;
	socklen_t *client_addrlen;

	printf("client mode selected\n");
	server = connect_server_socket("192.168.1.239",PORT);
	if (server < 0){
		fprintf(stderr,"ERROR: Could not connect.\n");
		return -1;
	}
	if (verbose){
		printf("Connection accepted, Starting handshake...\n");
	}
	
	printf("(Client)> "START_HANDSHAKE"\n");
	sendall(server,START_HANDSHAKE,strlen(START_HANDSHAKE)+1);

	buffer_length = recvall(server,&buffer);
	printf("(Server)> %s\n",buffer);
	if (buffer_length == strlen(COMPLETE_HANDSHAKE)+1 && strncmp(COMPLETE_HANDSHAKE, buffer, buffer_length) == 0){
		if (verbose){
			printf("Handshake completed succesfully.\n");
		}
	}else{
		close(server);
		fprintf(stderr,"ERROR: Handshake completion %s did not match %s or length %d did not match %d\n", buffer, COMPLETE_HANDSHAKE, buffer_length, strlen(COMPLETE_HANDSHAKE)+1);
		return -1;
	}

	printf("(Client)> "READY"\n");
	sendall(server,READY,strlen(READY)+1);

	while (active){
		buffer_length = recvall(server,&buffer);
		if (buffer_length < 1){
			fprintf(stderr,"Connection terminated prematurely.\n");
			return -1;
		}
		printf("(Server)> %s\n",buffer);
		if (buffer_length == strlen(KILL)+1 && strncmp(buffer, KILL, buffer_length) == 0){
			if (verbose){
				printf("Received kill signal, stopping.\n");
			}
			printf("(Client)> "CONFIRM"\n");
			sendall(server,CONFIRM,strlen(CONFIRM)+1);
			active = 0;
			break;
		}else if (buffer_length == strlen(SENDING_FILE)+1 && strncmp(buffer, SENDING_FILE, buffer_length) == 0){
			if (verbose){
				printf("File incoming...\n");
			}
			result = sendall(server,CONFIRM,strlen(CONFIRM)+1);
			if (result < 0){
				fprintf(stderr,"Could not send confirmation.\n");
				return -1;
			}
			//receive and write the file
			recv_file(server,"epic_file");
		}else{
			printf("Server response [%s](%d) did not match [%s](%d) or [%s](%d). Connection may be broken. Terminating\n",buffer,buffer_length,SENDING_FILE,strlen(SENDING_FILE)+1,KILL,strlen(KILL));
		}

		free(buffer);//allocated by recvall
	}

	//cleaing
	close(server);
	return 0;
}
