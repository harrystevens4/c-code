//Program for quickly discovering a device with specific hostname on the network
/*
Here is the standard
See all relevant structs and enums below

=== filtering ===

setting the hostname and/or service in the discover
will cause only servers with matching hostnames or services
to respond. setting the service to all '\0's will cause the service to be
ignored, and setting the hostname length to 0 will cause the hostname to be ignored

=== only one qd service running ===

client                      server
   |  qd_discover_packet      |
   |------------------------->|
   |                          |
   |  qd_response_packet      |
   |<-------------------------|
   |
   |
   \
   clients ip is {w.x.y.z}

   responses will only come from servers
   with matching hostnames to that set
   in the discover packet

=== multiple qd services running on one server ===

client                           server
   |  qd_discover_packet           |
   | {hostname = "desktop-abc"}    |
   |------------------------------>|
   |                               |
   |                               |\
   |       qd_response_packet      | |\
   |<------------------------------| | |
   |       qd_response_packet      | | |
   |<--------------------------------| |
   |       qd_repsonse_packet      | | |
   |<----------------------------------|
   |                               | | |
   |                               | |  \
   |                               | |   service 3
   |                               |  \
   |                               |   service 2
   |                                \
   \                                 service 1
   `desktop-abc` ip is {w.x.y.z}; service `TIME`
   `desktop-abc` ip is {w.x.y.z}; service `PWRSWTCH`
   `desktop-abc` ip is {w.x.y.z}; service `BUZZER`

   qd_recv_response() will receive one response

   calling qd_recv_response() repeatedly untill a timeout
   or untill you find a server with the correct service
   is a great way to use this.
   dont forget you can use poll() on the qd file descriptor
   to see if there is a response pending.

=== multiple qd services running on different servers

                                    "desktop-a"
                                    / "desktop-b" has service TIME
                                   |  / "desktop-c"
client                             | |  / "desktop-d" has service TIME
   |  qd_discover_packet           | | |  /
   |   {hostname_len = 0,          | | | |
   |   service = "TIME"}           | | | |
   |------------------------------>+-+-+-+
   |                               | | | |
   |       qd_response_packet      | | | |
   |<--------------------------------| | |
   |       qd_response_packet      | | | |
   |<------------------------------------|
   |                               | | | |
   |                               | | | |
   \
   `desktop-b` ip is {w.x.y.z}; service `TIME`
   `desktop-d` ip is {w.x.y.z}; service `TIME`

   setting the `hostname_len` to 0 will cause
   all available services on all available devices
   to respond, but setting the service here will
   mean only those services on those devices will

*/

#define SERVICE_LEN 12
//all fields are stored in network byte order (obviously)
struct __attribute__((packed)) qd_discover_packet {
	char hostname[255];
	uint8_t hostname_len;
	char service[SERVICE_LEN]; //only used when hostname_len = 0
};
struct __attribute__((packed)) qd_response_packet {
	char hostname[255];
	uint8_t hostname_len;
	char service[SERVICE_LEN]; //small string to identify what service is being offered
	struct sockaddr_storage address;
};

//============ client use ============
//------ low level ------
int qd_recv_response(int qdfd, struct qd_response_packet *response){
}
int qd_send_discover(int qdfd, struct qd_discover_packet *discover){
}
int qd_client_socket(){
}

//============ server use ============
//------ low level ------
int qd_send_response(int qdfd, struct qd_response_packet *response){
}
int qd_recv_discover(int qdfd, struct qd_discover_packet *discover){
}
int qd_server_socket(){
}
static void _qd_server_thread(void *service_p){
	//the stack is more goated than the heap
	char service[SERVICE_LEN] = {0};
	memcpy(service,service_p,SERVICE_LEN);
	free(service_p);
	//=== setup socket ===
	//=== listening loop ===
}
//------ high level ------
//spawns a thread that advertises the given service
pthread_t start_qd_server(const char service[SERVICE_LEN]){
	//prevent race condition where service dropped before thread starts
	char *service_copy = malloc(SERVICE_LEN); //i hope malloc is thread safe
	memcpy(service_copy,service,SERVICE_LEN);
	//create new thread
	pthread_t thread;
	int result = pthread_create(&thread,NULL,&_qd_server_thread,service_copy);
	//return
	return thread;
}
void stop_qd_server(pthread_t server_thread){
	pthread_cancel(server_thread);
	pthread_join(server_thread);
}
