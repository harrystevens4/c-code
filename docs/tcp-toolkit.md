
# Function overview

## `int make_server_socket(const char * port, int backlog)`

Create a socket, bind it to your local IP address and `const char * port`, and then start listening with a backlog of `int backlog`.

### return value

Returns socket file descriptor on success, and < 0 for failure

## `int connect_server_socket(char * host, char * port)`

Takes a `char * host` which can be either an IP address, a hostname, or domain name, and `char * port` as the port. It then connects to a socket at the specified host, and returns the socket file descriptor.

### return value

Returns socket file descriptor on success, and < 0 for failure

## `int sendall(int socket, char * buffer, size_t buffer_length)`

Paired with `recvall()`, it first sends the size of the buffer, and then continuously sends the buffer until it is fully received.
Does not require buffer to be null terminated.

### return value

Returns 0 on success, and < 0 for failure

## `size_t recvall(int socket, char * buffer)`

Companion function to `sendall()`, takes a char pointer and dynamically allocates it to fit the incoming buffer, returning the size of the buffer on completion.

### return value

Returns size of received buffer on success, and 0 for failure

## `int send_file(int socket, const char * filename)` and `int recv_file(int socket, const char * filename)`

They both take the socket file descriptor and a filename. `send_fille` takes the filename, reads from it and transmits it to a waiting `recv_file`, which creates the `const char * filename` file if necessary and writes the incoming data to it.

### return values

Returns 0 on success, and < 0 for failure

# Debug

If you want a more verbose output, it can be achieved by setting `extern int verbose_tcp_toolkit` to 1
