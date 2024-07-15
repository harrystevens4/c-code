#include <sys/socket.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "args.h"
#include <pthread.h>
#include "daemon-toolkit.h"
#define SOCKET_FD "/tmp/log-manager.socket"
int connect_daemon();
void *thread();
int start_daemon();
int main(int argc, char *argv[]){
	struct args args;
	parse_args(argc,argv,&args);
	lock_tty();
	printf("epic\n");
	unlock_tty();
	if (args.number_single<1){
		return connect_daemon();
	}
	switch (args.single[0]){
		case 'd': //daemon
			return start_daemon();
	}
}
int start_daemon(){
	const int buffer_size = LINE_MAX;
	int buffer[buffer_size];
	int server = make_named_socket(SOCKET_FD);
	int data_socket;
	if (listen(server,20) != 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}
	data_socket = accept(server,NULL,NULL);
	close_named_socket(server,SOCKET_FD);
	return 0;
}
void *thread(){
	return NULL;
}

int connect_daemon(){
	int data;
	int data_socket = connect_named_socket(SOCKET_FD);
	close(data_socket);
	while ((data=read(stdin, buffer, LINE_MAX))>0){//read from standard input (for pipes)
		printf("buffer: %c",buffer[0])
	}
	return 0;
}
