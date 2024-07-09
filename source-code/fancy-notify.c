#include "daemon-toolkit.h"
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#define SOCKET_FD "/tmp/fancy-notify.socket"
#define SIMPLE "simple notification"
#define KILL "kill"
#define AUDIO "audio"
struct simple_notifications{
	char **messages;
	int count;
};
struct audio_notifications{
	char **messages;
	int count;
};
volatile int stop = 0; //turned to 1 when ctr-c is sent to stop daemon gracefully
volatile int stop_count = 0; //number of stop signals received
pthread_mutex_t lock;

int force = 0; //from -f flag
struct simple_notifications simple_notifications;
volatile struct audio_notifications audio_notifications;

void handle_signal(int sig);
void audio_notification(const char *message);
int start_daemon();
void lock_mutex();
void unlock_mutex();
void kill_daemon();
int simple_notification(const char* text);
void *main_thread();

int main(int argc, char* argv[]){
	/* mutex init */
	if (pthread_mutex_init(&lock, NULL)!=0){
		fprintf(stderr,"could not initialise vital mutex\n");
	}
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
				if (arguments.number_other != 1){
					fprintf(stderr,"not enough args provided\n");
					exit(EXIT_FAILURE);
				}
				simple_notification((const char *)arguments.other[0]);
				break;
			case 'k': //kill daemon
				kill_daemon();
				break;
			case 'a': //audio
				audio_notification((const char *)arguments.other[0]);
				break;
		}
	}
	/* cleanup */
	return 0;
}
int start_daemon(){
	/* setup signal handler for SIGINT */
	signal(SIGINT, handle_signal);

	/* set up notification buffer */
	simple_notifications.count = 0;
	simple_notifications.messages = malloc(sizeof(char*));
	audio_notifications.count = 0;
	audio_notifications.messages = malloc(sizeof(char*));
	
	/* set up main thread for processing notifications */
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, main_thread, NULL);
	
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
		printf("listening for connections\n");
		data_socket = accept(server,NULL,NULL);
		printf("new connection\n");
		receive_string(data_socket,&buffer);
		printf("got new buffer of %s",buffer);

		/* simple notifications */
		if (strcmp(buffer,SIMPLE) == 0){
			receive_string(data_socket,&buffer);
			printf("aquiring simple notification lock\n");

			if (pthread_mutex_lock(&lock)!=0){
				fprintf(stderr,"critical mutex error in main_thread\n");
				exit(EXIT_FAILURE);
			}


			printf("aquired\n");
			simple_notifications.count++;
			simple_notifications.messages = realloc(simple_notifications.messages,sizeof(char*)*(simple_notifications.count));
			printf("allocating %d memory...\n",(int)(strlen(buffer)+1)*(int)sizeof(char));
			simple_notifications.messages[simple_notifications.count-1] = malloc((strlen(buffer)+1)*sizeof(char));
			sprintf(simple_notifications.messages[simple_notifications.count-1],"%s",buffer);
			printf("releasing...\n");
			if (pthread_mutex_unlock(&lock)!=0){
				fprintf(stderr,"critical mutex error in main\n");
				exit(EXIT_FAILURE);
			}
		/* kill */
		}else if (strcmp(buffer,KILL) == 0){
			printf("stopping daemon\n");
			stop = 1;

		/* audio notifications */
		}else if (strcmp(buffer,AUDIO) == 0){//audio notificaitons
			printf("getting audio string");
			receive_string(data_socket,&buffer);
			printf("got new audio notification of %s\n",buffer);
			lock_mutex();//safe zone
			printf("locked mutex\n");
			if (audio_notifications.count < 0){
				fprintf(stdout,"audio notification count below 0\n");
			}
			audio_notifications.count++;
			printf("reallocating audio notifications to %d\n",(int)sizeof(char*)*(audio_notifications.count));
			audio_notifications.messages = realloc(audio_notifications.messages,sizeof(char*)*(audio_notifications.count));
			printf("successfully realocated audio notifications\n");
			audio_notifications.messages[audio_notifications.count-1] = malloc(strlen(buffer)+1);
			sprintf(audio_notifications.messages[audio_notifications.count-1],"%s",buffer);
			unlock_mutex();//back to unsafe
			printf("unlocked mutex\n");
		}
		//clean up ready for next connection
		free(buffer);
		close(data_socket);
	}

	/* cleanup and success */
	close(server);
	remove(SOCKET_FD);
	/* simple notifications */
	for (int i = 0;i<simple_notifications.count;i++){

		free(simple_notifications.messages[i]);
	}
	free(simple_notifications.messages);
	/* audio notifications */
	for (int i = 0;i<audio_notifications.count;i++){
		free(audio_notifications.messages[i]);
	}
	free(audio_notifications.messages);

	pthread_mutex_destroy(&lock);
	return 0;
}
void *main_thread(){
	char *simple_notification_message;
	char *buffer;
	while (!stop){
		if (pthread_mutex_lock(&lock)!=0){//aquire lock
			fprintf(stderr,"critical mutex error in main_thread\n");
			exit(EXIT_FAILURE);
		}
		/* simple notifications */
		if (simple_notifications.count > 0){
			printf("------ SAFE ------\n");
			//printf("main_thread: processing simple notification...\n");
			simple_notification_message=simple_notifications.messages[simple_notifications.count-1]; //copy over message so we can release the lock and use a local copy
			simple_notifications.count--;
			printf("main_thread: creating notification %s\n",simple_notification_message);
			//printf("main_thread: done\n");
			//release lock as early as possible
			if (pthread_mutex_unlock(&lock)!=0){//release lock
				fprintf(stderr,"critical mutex error in main_thread\n");
				exit(EXIT_FAILURE);

			}
			buffer = malloc(strlen(simple_notification_message)+60);
			sprintf(buffer,"/usr/local/bin/simple-notification.py \"%s\"",simple_notification_message);
			system(buffer);
		}
		if (audio_notifications.count > 0){
			printf("------ SAFE ------\n");
			printf("main_thread: new audio notification detected\n");
			//lock_mutex();//start of safety //locked further up
			printf("main_thread: aquired lock\nnumber of audio notifications: %d\n",audio_notifications.count);
			if (audio_notifications.count > 1){ //dont realloc to array with 0 elements
				printf("main_thread: reallocating audio_notifications,messages to %d\n",(int)sizeof(audio_notifications.count-1));
				audio_notifications.messages = realloc(audio_notifications.messages,sizeof(char*)*(audio_notifications.count-1));
			}
			buffer = malloc(sizeof(char)*(strlen(audio_notifications.messages[audio_notifications.count-1])+20));//redundancy and space for the "spd-say"
			sprintf(buffer,"sudo -u harry /usr/bin/spd-say -w \"%s\"",audio_notifications.messages[audio_notifications.count-1]);//copy it over so we dont have to hold the lock longer then necesary
			free(audio_notifications.messages[audio_notifications.count-1]);
			audio_notifications.count--;
			unlock_mutex();//move to unsafe
			printf("main_thread: released lock, sending audio\n");
			system(buffer);// equivalent to spd-say "$message"
			printf("main_thread: sent audio\n");
			sleep(1);
		}
		pthread_mutex_unlock(&lock);//failsafe
		//sleep(2); //simulated delay
	}
	return NULL;
}
void audio_notification(const char *message){
	int data_socket = connect_named_socket(SOCKET_FD);
	send_string(data_socket,AUDIO);
	send_string(data_socket,message);
}
int simple_notification(const char* text){
	int data_socket = connect_named_socket(SOCKET_FD);
	send_string(data_socket,SIMPLE);
	send_string(data_socket,text);

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
void kill_daemon(){
	printf("attempting to kill daemon\n");
	int data_socket = connect_named_socket(SOCKET_FD);
	send_string(data_socket,KILL);
	close(data_socket);
}
void lock_mutex(){
	if (pthread_mutex_lock(&lock)!=0){
		fprintf(stderr,"critical mutex error\n");
		exit(EXIT_FAILURE);
	}
	printf("------ SAFE ------\n");
}
void unlock_mutex(){
	printf("------ UNSAFE ------\n");
	if (pthread_mutex_unlock(&lock)!=0){
		fprintf(stderr,"critical mutex error\n");
		exit(EXIT_FAILURE);
	}
}
