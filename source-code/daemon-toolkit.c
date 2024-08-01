#include <stddef.h>
#include <limits.h>
#include <pthread.h>	
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdlib.h>
#define END "END" //end of transmition
#define ACK "ACK" //acknowledgement signal

struct buffer{
        int buffer_length; //number of strings in the buffer
        int *lengths; //array of lengths corresponding to strings in buffer (not including null byte)
        char **buffer; //for storing the buffer of commands to be processed by the daemon
};

pthread_mutex_t tty_lock;
volatile int STOP = 0;
int non_lethal_errors;

void free_buffer(struct buffer *buffer){
	free(buffer->lengths);
	for (int i=0;i<buffer->buffer_length;i++){
		free(buffer->buffer[i]);
	}
	free(buffer->buffer);
}


int make_named_socket (const char *filename){
  struct sockaddr_un name;
  int sock;
  //size_t size;
  //int opt = 1;
  /* Create the socket. */
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
	perror ("socket");
	    if (non_lethal_errors){
		    return -1;
	    }else{
		exit (EXIT_FAILURE);
	    }
    }

  /* allow reuse of socket address */
  //if (setsockopt(sock, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))){perror("setsockopt");exit (EXIT_FAILURE);}
  /* Bind a name to the socket. */
  memset(&name, 0, sizeof(struct sockaddr_un));
  name.sun_family = AF_UNIX;
  strncpy (name.sun_path, filename, sizeof(name.sun_path)-1);
  //name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
 */
  //size = (offsetof (struct sockaddr_un, sun_path)+ strlen (name.sun_path));

  if (bind (sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un)) < 0)
    {
	perror ("bind");
	    if (non_lethal_errors){
		return -1;
	    }else{
      exit (EXIT_FAILURE);
	    }
    }
  if (chmod(filename,strtol("1777", 0, 8)) != 0){
	  fprintf(stderr,"Could not make socket accesable for all users.\n");
  }


  return sock;
}

int connect_named_socket (const char *filename){
  struct sockaddr_un name;
  int sock;
  //size_t size;
  //int opt = 1;
  /* Create the socket. */
  sock = socket (AF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
	    if (non_lethal_errors){
		    return -1;
	    }else{
      exit (EXIT_FAILURE);
	    }
    }

  /* allow reuse of socket address */
  //if (setsockopt(sock, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))){perror("setsockopt");exit (EXIT_FAILURE);}
  /* Bind a name to the socket. */
  memset(&name, 0, sizeof(struct sockaddr_un));
  name.sun_family = AF_UNIX;
  strncpy (name.sun_path, filename, sizeof (name.sun_path));
  //name.sun_path[sizeof (name.sun_path) - 1] = '\0';

  /* The size of the address is
     the offset of the start of the filename,
     plus its length (not including the terminating null byte).
     Alternatively you can just do:
     size = SUN_LEN (&name);
 */
  //size = (offsetof (struct sockaddr_un, sun_path)
  //        + strlen (name.sun_path));
  if(connect(sock,(const struct sockaddr *) &name,sizeof(struct sockaddr_un))<0){
	  perror("connect");
	  if (non_lethal_errors){
		  return -1;
	  }else{
	  exit(EXIT_FAILURE);
	  }
  }

  return sock;
}
int receive_string(int socket,char **string_buffer){
	const int buffer_size = 4;//we only need to receive END and one char buffers
	char buffer[buffer_size];
	int result;
	*string_buffer = calloc(1,sizeof(char));
	int index = 0;
	//printf("using socket %d\n",socket);
	for (;;){
		result = read(socket,buffer,buffer_size);
		if (result<0){
			perror("read");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		if (!(strncmp(buffer,END,strlen(END)))){
			//printf("got end of transmition signal\n");
			string_buffer[0][index] = '\0';
			break;
		}
		//printf("buffer received of %c, updating index %d\n",buffer[0],index);
		string_buffer[0][index] = buffer[0]; //string_buffer[0] acts as deference * operator
		index++;
		//printf("reallocating string_buffer to %d chars\n",index+1);
		*string_buffer = realloc(*string_buffer,sizeof(char)*(index+1));
		//printf("current string stands at %s, continuing to build\n",*string_buffer);
		sprintf(buffer,ACK);
		//printf("sending acknowledgement of %s\n",buffer);
		result = write(socket,buffer,4);//standard length for command verbs is 3 chars + \0
		if (result<0){
			perror("write");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		//printf("acknowledgement sent\n");
	}
	//printf("completed building string\n");
	return 1;
}
int send_string(int socket,const char *string){
	char buffer[4];
	int result;
	for (int index=0;index<strlen(string);index++){
		sprintf(buffer,"%c",string[index]);
		//printf("sending %c\n",buffer[0]);
		result = write(socket,buffer,4);
		if (result<0){
			perror("write");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		//printf("awaiting acknowledgement\n");
		result = read(socket,buffer,4);
		if (result<0){
			perror("read");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
	}	
	//printf("sending end of transmition\n");
	result = write(socket,END,4);
	if (result<0){
		perror("write");
		if (non_lethal_errors){
			return -1;
		}else{
			exit(EXIT_FAILURE);
		}
	}
	return 0;
}
int send_int(int socket, int number){
	int result;
	char buffer[4];
	int string_size = snprintf(NULL,0,"%d",number)+1;//calculate max buffer size
	//printf("max string size %d\n",string_size);
	char *string = calloc(string_size,sizeof(int));//does null termination for us
	sprintf(string,"%d",number);
	for (int i=0;i<string_size-1;i++){
		sprintf(buffer,"%c",string[i]);
		//printf("sending %c\n",string[i]);
		result = write(socket,buffer,2);
		if (result < 0){
			perror("write");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		result = read(socket,buffer,4);
		if (result < 0){
			perror("read");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		//printf("got acknowledgement of %s\n",buffer);
	}
	
	write(socket,END,4);
	/* cleanup */
	free(string);
	return 0;
}
int receive_int(int socket){
	int result;
	int number;
	char *err;
	char buffer[4];
	char *string = malloc(sizeof(char));//does null termination for us
	for (int i=0;;i++){
		result = read(socket,buffer,4);
		if (result < 0){
			perror("read");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		if (strncmp(END,buffer,3) == 0){
			string[i] = '\0';
			break;
		}
		//printf("received %c\n",buffer[0]);
		string = realloc(string,sizeof(char)*(i+2));
		string[i] = buffer[0];
		//printf("sending acknowledgement\n");
		result = write(socket,ACK,2);
		if (result < 0){
			perror("write");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
	}
	number = strtol(string,&err,10);
	/* cleanup */
	free(string);
	return number;
}

int close_named_socket(int socket,const char *filename){
	close(socket);
	remove(filename);
	return 0;
}
int lock_tty(){
	static int init = 0;
	//attempt to init tty_lock if it hasnt been already
	if (init == 0){
		if (pthread_mutex_init(&tty_lock,NULL) != 0){
			perror("mutex init:");
			if (non_lethal_errors){
				return -1;
			}else{
				exit(EXIT_FAILURE);
			}
		}
		init = 1;
	}
	int result = pthread_mutex_lock(&tty_lock);
	if (result != 0){
		fprintf(stderr,"Mutex error for tty_lock, could not lock.");
	}
	return result;
}
int unlock_tty(){
	int result = pthread_mutex_unlock(&tty_lock);
	if (result != 0){
		fprintf(stderr,"Mutex error for tty_lock, could not unlock.");
	}
	return result;
}
