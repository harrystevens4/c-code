#include "daemon-core.h"
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#define SOCKET_FD "/tmp/fancy-notify.socket"
#define SIMPLE "simple notification"
#define KILL "kill"
struct notifications{
	char **notifications;
	int count;
};
volatile int stop = 0; //turned to 1 when ctr-c is sent to stop daemon gracefully
volatile int stop_count = 0; //number of stop signals received
int force = 0; //from -f flag

void handle_signal(int sig);
int start_daemon();
int kill_daemon();
int simple_notification(const char* text);

int main(int argc, char* argv[]){
	/* argument processing */
	struct args arguments;
	parse_args(argc,argv,&arguments);
	if (argc < 2){
		fprintf(stderr,"No arguments provided. Refer to man-page for more info\n");
	}
	for (int i = 0;i<arguments.number_single;i++){
		switch(arguments.single[i]){
			case 'd': //daemon mode
				printf("starting daemon\n");
				if (start_daemon() != 0){
					fprintf(stderr,"Could not start daemon.\n");
					return 1;
				}
				break;
			case 'f':  //force
				force = 1;
				break;
			case 's': //simple type
				simple_notification("this is a notification");
			case 'k': //kill daemon
				kill_daemon();
		}
	}
	/* cleanup */
	return 0;
}
int start_daemon(){
	/* setup signal handler for SIGINT */
	signal(SIGINT, handle_signal);

	/* set up notification buffer */
	struct notifications notifications;
	notifications.count = 0;
	notifications.notifications = malloc(sizeof(char*));
	
	/* set up sockets */
	if (access(SOCKET_FD, F_OK) == 0){
		if (force){
			printf("overwriting previous socket\n");
			remove(SOCKET_FD);
		}else{
			fprintf(stderr,"Socket already exists, daemon may be running elsewhere. use -f to continue\n");
		}
	}
	int server = make_named_socket(SOCKET_FD);
	listen(server,1);
	system("chmod 777 "SOCKET_FD); //allow r+w for all users
	int data_socket;
	char *buffer;

	/* mainloop */
	while (!stop){
		data_socket = accept(server,NULL,NULL);
		buffer = receive_string(data_socket);
		if (strcmp(buffer,SIMPLE) == 0){
			printf("creating simple notification\n");
		}else if (strcmp(buffer,KILL) == 0){
			printf("stopping daemon\n");
			stop = 1;
		}
		//clean up ready for next connection
		free(buffer);
		close(data_socket);
	}

	/* cleanup and success */
	close(server);
	remove(SOCKET_FD);
	for (int i = 0;i<notifications.count;i++){
		free(notifications.notifications[i]);
	}
	free(notifications.notifications);
	return 0;
}
int simple_notification(const char* text){
	int data_socket = connect_named_socket(SOCKET_FD);
	send_string(data_socket,SIMPLE);

	/* cleanup */
	close(data_socket);
	return 0;
}
void handle_signal(int sig){
	signal(sig, SIG_IGN);
	switch (sig){
		case SIGINT:
			if (stop_count<1){
				fprintf(stderr,"SIGINT received\n");
				stop = 1;
				stop_count++;
			}else{
				fprintf(stderr,"Forced stop\n");
				exit(1);
			}
	}
	signal(sig, handle_signal);
}
int kill_daemon(){
	int data_socket = connect_named_socket(SOCKET_FD);
	send_string(data_socket,KILL);
	close(data_socket);
}
