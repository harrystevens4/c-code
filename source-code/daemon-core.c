#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include "daemon-core.h"
#include <stdlib.h>
#define END "END" //end of transmition
#define ACK "ACK" //acknowledgement signal

void free_buffer(struct buffer *buffer){
	free(buffer->lengths);
	for (int i=0;i<buffer->buffer_length;i++){
		free(buffer->buffer[i]);
	}
	free(buffer->buffer);
}

void free_locks(struct locks *locks){
	free(locks->lock);
	for (int i=0;i<locks->number_of_locks;i++){
		free(locks->lock_name[i]);
	}
	free(locks->lock_name);
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
      exit (EXIT_FAILURE);
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
      exit (EXIT_FAILURE);
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
      exit (EXIT_FAILURE);
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
	  exit(EXIT_FAILURE);
  }

  return sock;
}
char *receive_string(int socket){
	const int buffer_size = 4;//we only need to receive END and one char buffers
	char buffer[buffer_size];
	int result;
	char *string = malloc(sizeof(char));
	int index = 0;
	//printf("using socket %d\n",socket);
	for (;;){
		result = read(socket,buffer,buffer_size);
		if (result<0){
			perror("read");
			exit(EXIT_FAILURE);
		}
		if (!(strncmp(buffer,END,strlen(END)))){
			//printf("got end of transmition signal\n");
			string[index] = '\0';
			break;
		}
		//printf("buffer received of %c\n",buffer[0]);
		string[index] = buffer[0];
		index++;
		string = realloc(string,sizeof(char)*(index+1));
		//printf("current string stands at %s, continuing to build\n",string);
		sprintf(buffer,ACK);
		//printf("sending acknowledgement of %s\n",buffer);
		result = write(socket,buffer,4);//standard length for command verbs is 3 chars + \0
		if (result<0){
			perror("write");
			exit(EXIT_FAILURE);
		}
		//printf("acknowledgement sent\n");
	}
	return string;
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
			exit(EXIT_FAILURE);
		}
		//printf("awaiting acknowledgement\n");
		result = read(socket,buffer,4);
		if (result<0){
			perror("read");
			exit(EXIT_FAILURE);
		}
	}	
	//printf("sending end of transmition\n");
	result = write(socket,"END",4);
	if (result<0){
		perror("write");
		exit(EXIT_FAILURE);
	}
	return 0;
}
int close_named_socket(int socket,const char *filename){
	close(socket);
	remove(filename);
	return 0;
}
