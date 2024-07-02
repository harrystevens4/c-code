
# How to use `daemon-core.c` and coresponding `daemon-core.h`

## Sockets

To allow inter-process comunication, the `daemon-core.c` provides functions for Unix domain sockets in order to simplify the process. The following functions are provided:

### `int make_named_socket(const char *filename)`

This function creates a server socket, bound to the file path specified as a string. It then returns the socket created as an integer. Note: this function donly creates and binds the socket. You must set up listening and connection accepting. Note: you are recomended to have the file for the socket in the `/tmp` directory, due to having the property of reseting on restart, ensuring you will not have binding issues after a restart if the socket is not closed properly.
Returns 0 on success.

### `int connect_named_socket(const char *filename)`

This function connects to a server socket at the specified file path.
Returns the new data socket as an integer.

### `int close_named_socket(int socket, const char *filename)`

This function is for server sockets only, if you wish to close a client socket, use `close(socket)` provided by `unilib.h`. It removes the file used for the socket and closes the connection. Used to clean up after you are done with the server.  
Returns 0 on successfull completion.

### `int send_string(int socket, const char *string)`

This function sends the string provided by it, and does not care about string length.  
when you provide the socket and string to send, it will transmit each `char` one by one, and send an end signal once complete. This allows it to easily transmit any size of string without worry about buffer size.
Returns 0 on success.

### `char *receive_string(int socket)`

This is the counterpart to `send_string`. It will receive the string one char at a time and dynamicaly allocate a string to hold the final result after putting together the string. Note: you must `free(string)` after use to ensure that memory is not wasted.
Returns a string (`char*`) with the received transmition

## Locks 

### `struct locks`

Contains a dynamicaly allocatable 2d char array for holding names of locks, a dynamicaly allocatable array holding integets to store the status of locks and an integer for the number of locks. Designed to be flexible for multiple use cases.

### `void free_locks(struct locks *locks)`

This function should be provided a pointer your locks struct, and will free all the elements in `char **lock_name`and the array of `int *lock`

# How it works

## `make_named_socket()`

## `connect_named_socket()`

## `send_string`

## `receive_string()`

# Examples of how to use

