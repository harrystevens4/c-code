#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "args.h"
#include "daemon-toolkit.h"
#define SOCKET_FD "/tmp/mail-manager.socket"
#define MAIL_LOCATION "/var/spool/mail/system"
#define ERROR "------ ERROR ------\n"

struct mail{
	char **header;
	int count;
	char **body;
};

struct mail mail;
mail.count = 0;
static volatile int stop = 1;
pthread_mutex_t lock;
void *daemon_view_mail();
void daemon_receive_mail(int socket);
int client_send_mail();
void client_view_mail();
int start_daemon();
int dump_mail();
int load_mail();

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
int load_mail(){
	printf("attempting to load mail...\n");

	/* get a list of the mail files */
	FILE *mail_file;
	DIR *directory;
	struct dirent *dir;
	directory = opendir(MAIL_LOCATION);
	if (directory){
		while ((dir = readdir(directory)) != NULL){
			if (dir->d_type == DT_REG){//make sure it is a file
				printf("loading mail file %s\n", dir->d_name);
				/* loading mail into memory */
				if (pthread_lock_mutex(&lock) != 0){
					fprintf(stderr,ERROR"mutex locking error in load_mail()\n"ERROR);
					return 1;
				}
				mail_file = fopen(dir->d_name,"r");
				mail.header[mail.count] = realloc();
				fclose(mail_file);
				mail.count++;
				if (pthread_unlock_mutex(&lock) != 0){
					fprintf(stderr,ERROR"mutex unlocking error in load_mail()\n"ERROR);
					return 1;
				}
			}
		}
		closedir(directory);
	}
	printf("mail loaded\n");
	return 0;
}

int dump_mail(){
	printf("dumping mail...\n");
	/* check mail directory exists, and if not , create it */
	struct stat sb;
	if (! (stat(MAIL_LOCATION, &sb) == 0 && S_ISDIR(sb.st_mode))){
		if (mkdir(MAIL_LOCATION, 0777) != 0){
			fprintf(stderr,ERROR"could not create folder for mail storage\n"ERROR);
			return 1;
		}
	}
	/* dump each mail into its respective file */
	char filepath[20];
	FILE* mail_file;
	if (pthread_mutex_lock(&lock) != 0){
		fprintf(stderr,ERROR"could not lock mutex\n"ERROR);
		exit(EXIT_FAILURE);
	}
	for (int i=0;i<mail.count;i++){
		/* name mails after their index */
		sprintf(filepath,"%d",i);
		mail_file = fopen((const char *)filepath,"w");
		if (mail_file == NULL){
			fprintf(stderr,ERROR"could not open file %s\n"ERROR,filepath);
			return 1;
		}
		/* header and body are seperated by newline character */
		fprintf(mail_file,"%s\n%s",mail.header[i],mail.body[i]);
		//cleanup after each file
		fclose(mail_file);
	}
	if (pthread_mutex_unlock(&lock) != 0){
		fprintf(stderr,"could not unlock mutex\n");
		exit(EXIT_FAILURE);
	}
	printf("mail dumped\n");
	return 0;
}
int start_daemon(){
	pthread_mutex_init(&lock,NULL);
	pthread_t thread_id;
	
	/* load any mail stored in the mail files */
	if (load_mail() != 0){
		fprintf(stderr,"could not load mail\n");
	}

	int data_socket;
	int result = 0;
	char *buffer;
	char *header;
	char *body;
	int server = make_named_socket(SOCKET_FD);
	if (server < 0){
		fprintf(stderr,"could not create the server socket\n");
	}
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
	close_named_socket(server,SOCKET_FD);
	/* dump any unread mails into a file */
	result = dump_mail();
	pthread_mutex_destroy(&lock);
	return result;
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
	char *body;
	receive_string(socket,&header);
	printf("got header of %s\n",header);
	receive_string(socket,&body);
	printf("got body of %s\n",body);
	/* guaranteed atomicity of all operations */
	if (pthread_mutex_lock(&lock)<0){
		fprintf(stderr,"start_daemon: mutex failed to lock\n");
		exit(EXIT_FAILURE);
	}
	mail.count++;
	mail.header = realloc(mail.header,sizeof(char*)*(mail.count));
	mail.header[mail.count-1] = malloc(sizeof(char)*(strlen(header)+1));
	strcpy(mail.header[-1],header);
	mail.body = realloc(mail.body,sizeof(char*)*(mail.count));
	mail.body[mail.count-1] = malloc(sizeof(char)*(strlen(body)+1));
	strcpy(mail.body[-1],body);
	if (pthread_mutex_unlock(&lock)<0){
		fprintf(stderr,"start_daemon: mutex failed to unlock\n");
		exit(EXIT_FAILURE);
	}
	/* allow other threads to use hardware after this point */
}
int client_send_mail(){
	return 0;
}
