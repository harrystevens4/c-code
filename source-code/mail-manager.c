#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "args.h"
#include "daemon-toolkit.h"
#define SOCKET_FD "/tmp/mail-manager.socket"

struct mail{
	char **header;
	int count;
	char **body;
};

struct mail mail;
static volatile int stop = 1;
pthread_mutex_t lock;
void *daemon_view_mail();
void daemon_receive_mail(int socket);
int client_send_mail();
void client_view_mail();
int start_daemon();

int main(int argc, char *argv[]){
	struct args args;
	parse_args(argc,argv,&args);
	for (int i = 0;i<args.number_single;i++){
		if (args.single[i] == 'd'){
			return start_daemon();
		}
	}
	return 0;
}
int start_daemon(){
	pthread_mutex_init(&lock,NULL);
	pthread_t thread_id;
	int data_socket;
	char *buffer;
	char *header;
	char *body;
	int server = make_named_socket(SOCKET_FD);
	listen(server,10);
	while (!stop){
		data_socket = accept(server,NULL,NULL);
		receive_string(data_socket,&buffer);
		if (strcmp(buffer,"view") == 0){
			pthread_create(&thread_id,NULL,daemon_view_mail,(void *)(long)data_socket); //break of a seperate thread to deal with that
		}else if (strcmp(buffer,"send")){
			daemon_receive_mail(data_socket);
		}
		/* clean up connection */
		close(data_socket);
	}

	/* cleanup */
	pthread_mutex_lock(&lock);//extra safety if the threads havent finnished yet
	for (int i=0;i<mail.count;i++){
		free(mail.header[i]);
		free(mail.body[i]);
	}
	free(mail.header);
	free(mail.body);
	pthread_mutex_unlock(&lock);
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
void client_view_mail(){
	int data_socket = connect_named_socket(SOCKET_FD);
	close(data_socket);
}
void daemon_receive_mail(int socket){
	char *header;
	char *body
	receive_string(socket,header);
	printf("got header of %s\n",header);
	receive_string(socket,body);
	printf("got body of %s\n",body);
	/* guaranteed atomicity of all operations */
	if (pthread_mutex_lock(&lock)<0){
		fprintf(stderr,"start_daemon: mutex failed to lock\n");
		exit(EXIT_FAILURE);
	}
	mail.count++;
	mail.header = realloc(mail.header,sizeof(char*)*(mail.count));
	mail.header[count-1] = malloc(sizeof(char)*(strlen(header)+1));
	strcpy(mail.header[-1],header);
	if (pthread_mutex_unlock(&lock)<0){
		fprintf(stderr,"start_daemon: mutex failed to unlock\n");
		exit(EXIT_FAILURE);
	}
	/* allow other threads to use hardware after this point */
}
int client_send_mail(){
	return 0;
}
