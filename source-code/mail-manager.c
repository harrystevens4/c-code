#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "args.h"
#include "daemon-toolkit.h"
#define SOCKET_FD "/tmp/mail-manager.socket"

static volatile int stop = 1;
pthread_mutex_t lock;
void *daemon_view_mail(){
int start_daemon();

int main(int argc, char *argv[]){
	struct args args;
	parse_args(argc,argv,&args);

	return 0;
}
int start_daemon(){
	pthread_mutex_init(&lock,NULL);
	int data_socket;
	char *buffer;
	char *header;
	char *body;
	int server = make_named_socket(SOCKET_FD);
	listen(server,10);
	while (!stop){
		data_socket = accept(server,NULL,NULL);
		receive_string(data_socket,buffer);
		if (strcmp(buffer,"view") == 0){
			daemon_view_mail(); //break of a seperate thread to deal with that
		}
		/* clean up connection */
		close(data_socket);
	}

	/* cleanup */
	pthread_mutex_destroy(&lock);
	close_named_socket(server,SOCKET_FD);
	return 0;
}
void *daemon_view_mail(){
	while (!stop){
		/* guaranteed atomicity of all operations */
		if (pthread_mutex_lock(&lock)<0){
			fprintf(stderr,"start_daemon: mutex failed to lock\n");
			exit(EXIT_FAILURE);
		}
		if (pthread_mutex_unlock(&lock)<0){
			fprintf(stderr,"start_daemon: mutex failed to unlock\n");
			exit(EXIT_FAILURE);
		}
		/* allow other threads to use hardware after this point */
	}
	return NULL;
}
