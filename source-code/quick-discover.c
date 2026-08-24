//Program for quickly discovering a device with specific hostname on the network
//find documentation in header file
#include "quick-discover.h"

struct _qd_server_thread_arg {
	char service[SERVICE_LEN];
	volatile int init_status;
};

//all functions return 0 or > 0 on success and < 0 on error

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
static void _qd_server_thread(void *_qd_server_thread_arg){
	//==== setup ====
	struct _qd_server_thread_arg *arg = _qd_server_thread_arg;
	//extract the service
	char service[SERVICE_LEN] = {0};
	memcpy(service,&arg->service,SERVICE_LEN);
	//grab hostname
	char hostname[HOSTNAME_MAX_LEN+1] = {0};
	int result = gethostname(hostname,HOSTNAME_MAX_LEN);
	if (result < 0){
		arg->init_status = -errno;
		return;
	}
	uint8_t hostname_len = strlen(hostname);
	//get a socket
	int socket = qd_server_socket();
	if (socket < 0){
		arg->init_status = -errno;
		return;
	}
	//signal setup was successful
	arg->init_status = 1;
	//==== listening loop ====
	for (;;){
		//listen for discover requests
		struct qd_discover_packet discover_packet;
		int result = qd_recv_discover(socket,&discover_packet);
		if (result < 0){
			perror("qd_recv_discover");
			continue;
		}
		//filter any non relevant requests
		if (discover_packet.hostname_len != 0 ){
			if (strncmp(hostname,discover_packet.hostname,discover_packet.hostname_len) != 0) continue;
		}
		if (disover_packet.service[0] != '\0'){
			if (strncmp(discover_packet.service,service,SERVICE_LEN) != 0) continue;
		}
		//reply
		struct qd_response_packet response_packet = {0}
		response_packet.hostname_len = hostname_len;
		memcpy(&response_packet.hostname,hostname,hostname_len);
		//TODO
	}
}
//------ high level ------
//spawns a thread that advertises the given service
//returns 0 on failure and > 0 on success
pthread_t start_qd_server(const char *service){
	volatile struct _qd_server_thread_arg arg = {
		.init_status = 0,
	};
	memcpy(&arg.service,service,SERVICE_LEN);
	//create new thread
	pthread_t thread;
	int result = pthread_create(&thread,NULL,&_qd_server_thread,service_copy);
	//TODO this could definitely be done better
	//spin untill thread has initialised
	while (arg.init_status == 0);
	//check initialisation status
	if (arg.init_status < 0){
		errno = -arg.init_status;
		return 0;
	}else {
		//return
		return thread;
	}
}
void stop_qd_server(pthread_t server_thread){
	pthread_cancel(server_thread);
	pthread_join(server_thread);
}
