#include "daemon-core.h"
#include "args.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#define SERVER_FD "/tmp/harry/sockets/lock_manager_server.sock"
#define PORT 9000
#define BUFFER_SIZE 12
int start_daemon();
int acquire_lock(char *lock_name);
void kill_daemon();
int main(int argc, char *argv[]){
	/* initialisation */
	int result;
	struct args arguments;
	parse_args(argc,argv,&arguments);
	for (int single=0;single<arguments.number_single;single++){
		printf("proccessing arg %c\n",arguments.single[single]);
		switch (arguments.single[single]){
			case 's': //start daemon
				printf("starting daemon\n");
				result = start_daemon();
				if (result != 0){
					fprintf(stderr,"could not start daemon\n");
				}else{
					printf("server terminated with no errors\n");
					return 0;
				}
				break;
			case 'a': //aquire lock
				 if (arguments.number_other<1){
					 fprintf(stderr,"name not specified\n");
					 return 1;
				 }
				 acquire_lock(arguments.other[0]);
				 break;
			case 'k': //kill daemon
				  kill_daemon();
				  break;
		}
	}


	/* cleanup */
	free_args(&arguments);
	return 0;
}
int start_daemon(){
	/* locks */
	struct locks locks;
	locks.lock_name = malloc(sizeof(char*));
	locks.number_of_locks = 0;
	int lock_status; //0 for available 1 for unavailable
	/* socket */
	int server = make_named_socket(SERVER_FD);
	int data_socket;
	char kill[] = "kill";
	char command[BUFFER_SIZE];
	int result; //stores return values
	char buffer[BUFFER_SIZE];
	char *name;
	name = malloc(sizeof(char)*1);
	char confirm[] = "_null_";
	printf("created socket\n");
	/* enable listening for the server socket */
	if (listen(server,2)<0){ //queue size of 2
		perror("listen");
		exit(EXIT_FAILURE);
	}
	/* proccessing loop */
	for(;;){ //indefinetly process connecitons
		/* receive command */
		data_socket = accept(server,NULL,NULL);
		if (data_socket < 0){
			perror("accept");
			exit(EXIT_FAILURE);
		}
		result = read(data_socket,buffer,BUFFER_SIZE);
		if (result < 0){
			perror("read");
			exit(EXIT_FAILURE);
		}
		strcpy(command,buffer);
		printf("received command of %s\n",command);
		write(data_socket,confirm,7);
		int i = 0;
		for (;;){
			i++;
			read(data_socket,buffer,BUFFER_SIZE);
			if (!strncmp(buffer,"END",3)){
				break;
			}
			name = realloc(name,(sizeof(char)*i)+1);
			name[i-1] = buffer[0];

			write(data_socket,confirm,7);
		}
		name[i] = '\0';
		printf("received name of %s\n",name);
		close(data_socket);
		//break;
		/* command processing */
		if (!strncmp(command,kill,4)){ //kill the daemon
			break;
		}
		if (!strncmp(command,"acquire",7)){
			printf("checking lock status of lock named %s\n",name);
			for (int i=0;i<locks.number_of_locks;i++){
				if(!strncmp(locks.lock_name[i],name,strlen(name))){
					lock_status = 1;
				}
			}
			if (!(lock_status)){
				printf("lock available\n");
				locks.number_of_locks++;
				locks.lock_name = realloc(locks.lock_name,sizeof(char*)*locks.number_of_locks);
				locks.lock_name[locks.number_of_locks-1] = malloc(sizeof(char)*(strlen(name)+1));
				strcpy(locks.lock_name[locks.number_of_locks-1],name);
				printf("lock created\n");
				lock_status = 0;
			}else{
				printf("lock unavailable\n");
				lock_status = 1;
			}
		}
		printf("sending return status of %d\n",lock_status);
		write(data_socket,buffer,BUFFER_SIZE);
	}
	/* cleanup */
	printf("cleaning up\n");
	close(server);
	remove(SERVER_FD);
	free(name);
	//exit(EXIT_SUCCESS);
	return 0;
}
int acquire_lock(char *lock_name){
	char buffer[BUFFER_SIZE];
	int result;
	int data_socket;
	printf("attempting to aquire lock of name %s\n",lock_name);
	/* connect to server */
	data_socket = connect_named_socket(SERVER_FD);
	/* let the daemon know we want to aquire a lock */
	sprintf(buffer,"acquire");
	result = write(data_socket,buffer,BUFFER_SIZE);
	if (result<0){
		perror("write");
		exit(EXIT_FAILURE);
	}
	result = read(data_socket,buffer,BUFFER_SIZE);//confirmation of receive
	/* transmit the name of the lock */
	for (int i=0;i<strlen(lock_name);i++){
		printf("sending %c\n",lock_name[i]);
		buffer[0] = lock_name[i];
		result = write(data_socket,buffer,BUFFER_SIZE);
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
		result = read(data_socket,buffer,BUFFER_SIZE);//confirmation of receive
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
	printf("transmiting end of transmition\n");
	sprintf(buffer,"END");//end of transmition signal
	result = write(data_socket,buffer,BUFFER_SIZE);

	if (result<0){
		perror("write");
		exit(EXIT_FAILURE);
	}
	read(data_socket,buffer,BUFFER_SIZE);
	int return_status = buffer[0];
	printf("returned with status %d\n",return_status);

	/* clean up */
	printf("cleaning up\n");
	close(data_socket);
	return 0;
}
void kill_daemon(){
	char buffer[BUFFER_SIZE];
	int result;
	int data_socket;
	char lock_name[] = "die";
	printf("sending kill signal\n");
	/* connect to server */
	data_socket = connect_named_socket(SERVER_FD);
	/* let the daemon know we want to aquire a lock */
	sprintf(buffer,"kill");
	result = write(data_socket,buffer,BUFFER_SIZE);
	if (result<0){
		perror("write");
		exit(EXIT_FAILURE);
	}
	result = read(data_socket,buffer,BUFFER_SIZE);//confirmation of receive
	/* transmit the name of the lock */
	for (int i=0;i<strlen(lock_name);i++){
		printf("sending %c\n",lock_name[i]);
		buffer[0] = lock_name[i];
		result = write(data_socket,buffer,BUFFER_SIZE);
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
		result = read(data_socket,buffer,BUFFER_SIZE);//confirmation of receive
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
	}
	printf("transmiting end of transmition\n");
	sprintf(buffer,"END");//end of transmition signal
	result = write(data_socket,buffer,BUFFER_SIZE);

	if (result<0){
		perror("write");
		exit(EXIT_FAILURE);
	}
	printf("cleaning up\n");
	close(data_socket);
}
