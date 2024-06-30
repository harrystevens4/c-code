#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "daemon-core.h"
#include <stdlib.h>

void free_buffer(struct buffer *buffer){
	free(buffer->lengths);
	for (int i=0;i<buffer->buffer_length;i++){
		free(buffer->buffer[i]);
	}
	free(buffer->buffer);
}

void free_locks(struct locks *locks){
	free(locks->lock);
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
