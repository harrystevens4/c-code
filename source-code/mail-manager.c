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
static volatile int stop = 0;
pthread_mutex_t lock;
void *daemon_view_mail();
void daemon_receive_mail(int socket);
int client_send_mail(struct args args);
void client_view_mail();
int start_daemon();
int dump_mail();
int load_mail();
void kill_daemon();

int main(int argc, char *argv[]){

	struct args args;
	parse_args(argc,argv,&args);
	for (int i = 0;i<args.number_single;i++){
		if (args.single[i] == 'd'){
			return start_daemon();
		}
		if (args.single[i] == 'd'){
			kill_daemon();
		}
	}
	if(args.number_single == 0){
		return client_send_mail(args);
	}
	free_args(&args);
	return 0;
}
int load_mail(){
	printf("attempting to load mail...\n");
	int i;
	char buffer;
	char mail_file_path[280];

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
				if (pthread_mutex_lock(&lock) != 0){
					fprintf(stderr,ERROR"mutex locking error in load_mail()\n"ERROR);
					return 1;
				}
				sprintf(mail_file_path,"%s/%s",MAIL_LOCATION,dir->d_name);
				mail_file = fopen(mail_file_path,"r");
				if (mail_file == NULL){
					fprintf(stderr,ERROR"could not open file\n"ERROR);
					return 1;
				}
				//printf("reallocating header and body...\n");
				mail.header = realloc(mail.header,sizeof(char*)*(mail.count+1));
				mail.body = realloc(mail.body,sizeof(char*)*(mail.count+1));
				//printf("allocating header and body %d..\n",mail.count);
				mail.header[mail.count] = malloc(sizeof(char));
				mail.body[mail.count] = malloc(sizeof(char));
				for (i = 0;;i++){
					//printf("reallocating mail header %d...\n",mail.count);
					mail.header[mail.count] = realloc(mail.header[mail.count],sizeof(char)*(i+1));
					//printf("getting character...\n");
					buffer = fgetc(mail_file);
					//printf("got char of %c\n",buffer);
					if (feof(mail_file)){
						fprintf(stderr,ERROR"expected newline but got end of file. possible mail corruption\n"ERROR);
						return 1;
					}
					if (buffer != '\n'){
						mail.header[mail.count][i] = buffer;
					}else{
						mail.header[mail.count][i] = '\0';
						break;
					}
				}
				//printf("allocating body %d...\n",mail.count);
				mail.body[mail.count] = malloc(sizeof(char));
				for (i=0;;i++){
					//printf("reallocating mail body...\n");
					mail.body = realloc(mail.body,sizeof(char*)*(mail.count+1));
					buffer = fgetc(mail_file);
					//printf("got %c\n",buffer);
					if (feof(mail_file)){
						//printf("reached end of file\n");
						mail.body[mail.count][i] = '\0';
						break;
					}
					mail.body[mail.count][i] = buffer;
				}
				fclose(mail_file);
				mail.count++;
				printf("mail count now %d\n",mail.count);
				printf("--- header ---\n%s\n--- body ---\n%s",mail.header[mail.count-1],mail.body[mail.count-1]);
				if (pthread_mutex_unlock(&lock) != 0){
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
	char filepath[180];
	FILE* mail_file;
	if (pthread_mutex_lock(&lock) != 0){
		fprintf(stderr,ERROR"could not lock mutex\n"ERROR);
		exit(EXIT_FAILURE);
	}
	for (int i=0;i<mail.count;i++){
		/* name mails after their index */
		sprintf(filepath,"%s/%d",MAIL_LOCATION,i);
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
	mail.count = 0;
	mail.header = malloc(sizeof(char*));
	mail.body = malloc(sizeof(char*));
	pthread_mutex_init(&lock,NULL);
	pthread_t thread_id;
	
	/* load any mail stored in the mail files */
	if (load_mail() != 0){
		fprintf(stderr,ERROR"could not load mail\n"ERROR);
		return 1;
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
		}else if (strcmp(buffer,"send") == 0){
			daemon_receive_mail(data_socket);
		}else if (strcmp(buffer,"kill") == 0){
			stop = 1;
		}
		/* clean up connection */
		close(data_socket);
	}

	/* dump any unread mails into a file */
	result = dump_mail();

	/* cleanup */
	pthread_mutex_lock(&lock);//extra safety if the threads havent finnished yet
	printf("cleaning up %d mails...\n",mail.count);
	for (int i=0;i<mail.count;i++){
		printf("freeing mail header %d\n",i);
		free(mail.header[i]);
		printf("freeing mail body %d\n",i);
		free(mail.body[i]);
	}
	printf("freeing structs...\n");
	free(mail.header);
	free(mail.body);
	printf("closing socket...\n");
	pthread_mutex_unlock(&lock);
	close_named_socket(server,SOCKET_FD);
	printf("cleanup done\n");
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
	printf("incrementing mail count...\n");
	mail.count++;
	printf("copying header...\n");
	mail.header = realloc(mail.header,sizeof(char*)*(mail.count));
	mail.header[mail.count-1] = malloc(sizeof(char)*(strlen(header)+1));
	strcpy(mail.header[mail.count-1],header);
	printf("copying body...\n");
	mail.body = realloc(mail.body,sizeof(char*)*(mail.count));
	mail.body[mail.count-1] = malloc(sizeof(char)*(strlen(body)+1));
	strcpy(mail.body[mail.count-1],body);
	if (pthread_mutex_unlock(&lock)<0){
		fprintf(stderr,"start_daemon: mutex failed to unlock\n");
		exit(EXIT_FAILURE);
	}
	/* allow other threads to use hardware after this point */
	printf("cleaning up...\n");
	free(header);//cleanup
	free(body);
	printf("successfully got new mail!\n");
}
int client_send_mail(struct args args){
	if (args.number_other != 2){
		fprintf(stderr,"Expected 2 arguments of [header] and [body], but got %d\n",args.number_other);
		return 1;
	}
	printf("collecting mail from command line args...\n");
	int socket;
	char *header;
	char *body;
	printf("allocating space...\n");
	header = malloc(sizeof(char)*(strlen(args.other[0])+1));
	body = malloc(sizeof(char)*(strlen(args.other[1])+1));
	printf("copying args...\n");
	strcpy(header,args.other[0]);
	strcpy(body,args.other[1]);
	printf("connecting to daemon...\n");
	socket = connect_named_socket(SOCKET_FD);
	printf("sending data...\n");
	send_string(socket,"send");
	send_string(socket,(const char *)header);
	send_string(socket,(const char *)body);
	printf("freeing allocated memory...\n");
	free(header);
	free(body);
	printf("mail sent!\n");
	return 0;
}
void kill_daemon(){
	int socket = connect_named_socket(SOCKET_FD);
	send_string(socket,"kill");
}
