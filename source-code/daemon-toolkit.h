#ifndef core_h_
#define core_h_
struct buffer{
	int buffer_length; //number of strings in the buffer
	int *lengths; //array of lengths corresponding to strings in buffer (not including null byte)
	char **buffer; //for storing the buffer of commands to be processed by the daemon
};

void free_buffer(struct buffer *buffer);
int make_named_socket(const char *filename);
int connect_named_socket(const char *filename);
int receive_string(int socket, char **string_buffer); //the user is responsible for defining and freeing string
int send_string(int socket, const char *string);
int close_named_socket(int socket,const char *filename);
#endif
