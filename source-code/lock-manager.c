#include "daemon-core.h"
#include "args.h"
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#define SERVER_FD "/tmp/lock_manager_server.sock"
#define PORT 9000
#define BUFFER_SIZE 12
int start_daemon();
int acquire_lock(char *lock_name);
int query_lock(char *lock_name);
int release_lock(char *lock_name);
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
				 result = acquire_lock(arguments.other[0]);
				 if (result){
					 printf("lock taken\n");
				 }else{
					 printf("successfuly aquired lock\n");
				 }
				 return result;
			case 'k': //kill daemon
				  kill_daemon();
				  break;
			case 'q':
				  if (arguments.number_other<1){
					fprintf(stderr,"name not specified\n");
					return 1;
				  }
				  result = query_lock(arguments.other[0]);
				  if (result){//returns 1 if taken and 0 if free
					  printf("lock is taken\n");
				  }else{
					  printf("lock is free\n");
				  }
				  return result;
				  break;
			case 'r':
				  if (arguments.number_other<1){
					fprintf(stderr,"name not specified\n");
					return 1;
				  }
				  result = release_lock(arguments.other[0]);
				  if (result){//returns 1 if taken and 0 if free
					  printf("lock could not be release\n");
				  }else{
					  printf("successful lock release\n");
				  }
				  return result;
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
	int lock_index;
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
		memset(buffer,0,sizeof(buffer));
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
		memset(name,0,sizeof(name));
		printf("current value for name %s\n",name);
		for (;;){
			i++;
			read(data_socket,buffer,BUFFER_SIZE);
			if (!strncmp(buffer,"END",3)){
				memset(buffer,0,sizeof(buffer));
				break;
			}
			name = realloc(name,(sizeof(char)*i)+1);
			name[i-1] = buffer[0];
			memset(buffer,0,sizeof(buffer));

			write(data_socket,confirm,7);
		}
		name[i-1] = '\0';
		printf("received name of %s\n",name);
		//break;
		/* command processing */
		if (!strncmp(command,kill,4)){ //kill the daemon
			break;
		}
		lock_status = 0;
		if (!strncmp(command,"acquire",7)){
			printf("checking locks for a match to %s...\n",name);
			for (int i=0;i<locks.number_of_locks;i++){
				printf("\n%s",locks.lock_name[i]);
				if(!strncmp(locks.lock_name[i],name,strlen(name)+1)){
					lock_status = 1;
					printf(" - match\n");
					break;
				}
			}
			printf("\n\n");
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
		if (!strncmp(command,"query",5)){
			printf("checking lock status of lock named %s\n",name);
			for (int i=0;i<locks.number_of_locks;i++){
				if(!strncmp(locks.lock_name[i],name,strlen(name))){
					lock_status = 1;
				}else{
					lock_status = 0;
				}
			}
		}
		sprintf(buffer,"%d",lock_status);
		printf("sending return status of %s\n",buffer);
		result = write(data_socket,buffer,BUFFER_SIZE);
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
		if (!strncmp(command,"release",7)){
			printf("checking locks for a match to %s...\n",name);
			for (int i=0;i<locks.number_of_locks;i++){
				printf("\n%s",locks.lock_name[i]);
				if(!strncmp(locks.lock_name[i],name,strlen(name)+1)){
					lock_status = 1;
					lock_index = i;
					printf(" - match\n");
					break;
				}
			}
			printf("\n\n");
			if ((lock_status)){
				printf("lock exists\n");
				locks.number_of_locks--;
				locks.lock_name[lock_index] = malloc(sizeof(char)*(strlen(locks.lock_name[locks.number_of_locks])+1));//resize old lock index in prep to move the last index here
				strcpy(locks.lock_name[lock_index],locks.lock_name[locks.number_of_locks]);// copy the last entry to the old locks index
				locks.lock_name = realloc(locks.lock_name,sizeof(char*)*locks.number_of_locks);// shrink lock array by 1 (to remove last entry)
				printf("lock removed\n");
				lock_status = 0; //1 and 0 are swapped here in favour of returning 0 for a successfull removal over the sending the lock status as 1
			}else{
				printf("lock doesnt exist\n");
				lock_status = 1;
			}
		}

	}
	/* cleanup */
	printf("cleaning up\n");
	close(server);
	close(data_socket);
	remove(SERVER_FD);
	free(name);
	//exit(EXIT_SUCCESS);
	return 0;
}
int query_lock(char *lock_name){
	char buffer[BUFFER_SIZE];
	int return_status = 2;
	int result;
	int data_socket;
	printf("attempting to check status of lock of name %s\n",lock_name);
	/* connect to server */
	data_socket = connect_named_socket(SERVER_FD);
	/* let the daemon know we want to aquire a lock */
	sprintf(buffer,"query");
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
	for (;;){ //loop untill we receive an int and not "END"
		read(data_socket,buffer,BUFFER_SIZE);
		printf("got buffer for status %s\n",buffer);
		if (strncmp(buffer,"END",3)){
			break;
		}
	}
	return_status = atoi(buffer);
	printf("returned with status %d\n",return_status);

	/* clean up */
	printf("cleaning up\n");
	close(data_socket);
	return return_status;
}
int acquire_lock(char *lock_name){
	char buffer[BUFFER_SIZE];
	int return_status = 2;
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
	for (;;){ //loop untill we receive an int and not "END"
		read(data_socket,buffer,BUFFER_SIZE);
		printf("got buffer for status %s\n",buffer);
		if (strncmp(buffer,"END",3)){
			break;
		}
	}
	return_status = atoi(buffer);
	printf("returned with status %d\n",return_status);

	/* clean up */
	printf("cleaning up\n");
	close(data_socket);
	return return_status;
}
int release_lock(char *lock_name){
	char buffer[BUFFER_SIZE];
	int return_status = 2;
	int result;
	int data_socket;
	printf("attempting to release lock of name %s\n",lock_name);
	/* connect to server */
	data_socket = connect_named_socket(SERVER_FD);
	/* let the daemon know we want to aquire a lock */
	sprintf(buffer,"release");
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
	for (;;){ //loop untill we receive an int and not "END"
		read(data_socket,buffer,BUFFER_SIZE);
		printf("got buffer for status %s\n",buffer);
		if (strncmp(buffer,"END",3)){
			break;
		}
	}
	return_status = atoi(buffer);
	printf("returned with status %d\n",return_status);

	/* clean up */
	printf("cleaning up\n");
	close(data_socket);
	return return_status;
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
