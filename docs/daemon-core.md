
# How to use `daemon-core.c` and corresponding `daemon-core.h`

## Errors

Most of the socket functions, when they experience an error, will print an error to `stdout`, and if critical enough to affect the return value, will `exit(EXIT_FAILURE)`. If this is not desired, the `extern int non_lethal_errors` can be set to 1, and will result in functions returning -1 or null depending on if they return an int or not, allowing better control of error handling.

## Sockets

To allow inter-process communication, the `daemon-core.c` provides functions for Unix domain sockets in order to simplify the process. The following functions are provided:

### `int make_named_socket(const char *filename)`

This function creates a server socket, bound to the file path specified as a string. It then returns the socket created as an integer. Note: this function only creates and binds the socket. You must set up listening and connection accepting. Note: you are recommended to have the file for the socket in the `/tmp` directory, due to having the property of resetting on restart, ensuring you will not have binding issues after a restart if the socket is not closed properly.

Returns 0 on success.

### `int connect_named_socket(const char *filename)`

This function connects to a server socket at the specified file path.

Returns the new data socket as an integer.

### `int close_named_socket(int socket, const char *filename)`

This function is for server sockets only, if you wish to close a client socket, use `close(socket)` provided by `unilib.h`. It removes the file used for the socket and closes the connection. Used to clean up after you are done with the server.  

Returns 0 on successful completion.

### `int send_string(int socket, const char *string)`

This function sends the string provided by it, and does not care about string length.  
when you provide the socket and string to send, it will transmit each `char` one by one, and send an end signal once complete. This allows it to easily transmit any size of string without worry about buffer size.

Returns 0 on success.

### `char *receive_string(int socket)`

This is the counterpart to `send_string`. It will receive the string one char at a time and dynamicaly allocate a string to hold the final result after putting together the string. Note: you must `free(string)` after use to ensure that memory is not wasted.

Returns a string (`char*`) with the received transmission

## `int send_int(int socket, int number)`

Sends the integer `number` to the provided socket. Paired with `receive_int` to communicate integers over a socket.
Returns 0 on success.

### `int receive_int(int socket)`

This function will receive an integer on a provided socket. Similar to `receive_string` except it returns an integer.
Returns -1 on failure

## Locks 

### `struct locks`

Contains a dynamicaly allocatable 2d char array for holding names of locks, a dynamicaly allocatable array holding integets to store the status of locks and an integer for the number of locks. Designed to be flexible for multiple use cases.

### `void free_locks(struct locks *locks)`

This function should be provided a pointer your locks struct, and will free all the elements in `char **lock_name`and the array of `int *lock`

# How it works (contracted for simplicity)

## `int make_named_socket (const char *filename)`

	/* initailise the struct */
	struct sockaddr_un name;
	...
	/* Create the socket. */
	int sock;
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	...
	/* bind a name to the socket. */
	name.sun_family = AF_UNIX;
	strncpy (name.sun_path, filename, sizeof(name.sun_path)-1);
	...
	/* bind socket to file */
	bind (sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un))
	...
	return sock;

## `int connect_named_socket (const char *filename)`
	
	/* initialisation of variables */
	struct sockaddr_un name;
	int sock;
	...
	/* Create the socket. */
	sock = socket (AF_UNIX, SOCK_STREAM, 0);
	...
	/* Bind a name to the socket. */
	memset(&name, 0, sizeof(struct sockaddr_un));
	name.sun_family = AF_UNIX;
	strncpy (name.sun_path, filename, sizeof (name.sun_path));
	...
	/* connect to the server socket */
	connect(sock,(const struct sockaddr *) &name,sizeof(struct sockaddr_un))
	...
	return sock;

## `int send_string(int socket,const char *string)`

	/* variable initialisation */
        char buffer[4];
        int result;

	/* itterate through each char in the string */
        for (int index=0;index<strlen(string);index++){
                sprintf(buffer,"%c",string[index]);
                write(socket,buffer,4);
		...
		/* send acknowledgement */
                read(socket,buffer,4);
		...
        }
	/* tell the other side transmition is complete */
        write(socket,"END",4);
        return 0;

## `char *receive_string(int socket)`

	/* variable initialisation */
        const int buffer_size = 4;//we only need to receive END and one char buffers
        char buffer[buffer_size];
        int index = 0;
	...
	/* loop untill we receive END */
        for (;;){
                read(socket,buffer,buffer_size);
                if (!(strncmp(buffer,END,strlen(END)))){ //END is a macro here to "END"
                        /* end the string once the END signal is received */
			string[index] = '\0';
                        break;
                }
                string[index] = buffer[0];
                index++;
                string = realloc(string,sizeof(char)*(index+1));
                sprintf(buffer,ACK); //ACK is a macro here to "ACK"
                write(socket,buffer,4);//standard length for command verbs is 3 chars + \0
		...
        }
        return string;

# Examples of how to use

	#include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <sys/un.h>
	#include <sys/socket.h>
	#include "daemon-core.h"
	#include "args.h"
	#define SOCKET_FD "/tmp/socket_test.socket"
	int start_server();
	int start_client();
	int main(int argc, char *argv[]){
		struct args args;
		parse_args(argc,argv,&args);
		if (args.single[0] == 's'){ //check for -s option from when binary was executed
			start_server();
		}else{
			start_client();
		}
		free_args(&args);
		return 0;
	}
	int start_server(){
		remove(SOCKET_FD);
		int server;
		int client;
		
		/* socket */
		printf("creating socket\n");
		server = make_named_socket(SOCKET_FD);
		printf("listening\n");
		listen(server,2);
		client = accept(server,NULL,NULL);
		send_string(client,"based or cringe");
		printf("closing socket\n");

		/* cleanup */
		close(client);
		close_named_socket(server,SOCKET_FD);
		return 0;
	}
	int start_client(){
		int server;
		char *string;

		/* socket */
		printf("attempting to connect to server\n");
		server = connect_named_socket(SOCKET_FD);
		string = receive_string(server);
		printf("got %s\n",string);
		
		/* cleanup */
		free(string);
		return 0;
	}
