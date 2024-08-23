#ifndef TCP_TOOLKIT_H //include guard
#define TCP_TOOLKIT_H

int make_server_socket(const char * port, int backlog);//sets up a listening server socket
int connect_server_socket(char * host, char * port);
int sendall(int socket, char * buffer, size_t buffer_length);
size_t recvall(int socket, char **buffer);// dynamicaly allocates memory for the buffer and returns the size of the buffer as an integer
int send_file(int socket, const char * filename);
int recv_file(int socket, const char * filename);

extern int verbose_tcp_toolkit; //debugging info from tcp-toolkit functions
extern int silent_errors;

#endif
