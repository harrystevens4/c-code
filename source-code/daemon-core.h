#ifndef core_h_
#define core_h_
struct buffer{
	int buffer_length; //number of strings in the buffer
	int *lengths; //array of lengths corresponding to strings in buffer (not including null byte)
	char **buffer; //for storing the buffer of commands to be processed by the daemon
};
struct locks{
	int number_of_locks;
	char **lock_name; //aray of strings for named locks
	int * lock; //an array for locks (0 for unlocked, 1 for locked)
};

void free_buffer(struct buffer *buffer);
void free_locks(struct locks *locks);
int make_named_socket(const char *filename);
int connect_named_socket(const char *filename);
char *receive_string(int socket); //the user is responsible for defining and freeing string
int send_string(int socket, const char *string);
int close_named_socket(int socket,const char *filename);
#endif
