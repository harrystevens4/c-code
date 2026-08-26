//Program for quickly discovering a device with specific hostname on the network
//find documentation in header file
#include "quick-discover.h"
#include <ifaddrs.h>
#include <pthread.h>

struct _qd_server_thread_arg {
	char service[SERVICE_LEN];
	pthread_mutex_t init_event_mutex;
	pthread_cond_t init_event_condition;
	int init_status;
};

//will close the file descriptor when its initial assignment goes out of scope
static void _close_shim(void *arg){
	int fd = *(int *)arg;
	if (fd < 0) return;
	close(fd);
}
#define FD_SCOPED \
	__attribute__((__cleanup__(_close_shim))) int

//all functions return 0 or > 0 on success and < 0 on error

//============ client use ============
//------ low level ------
int qd_recv_response(int qdfd, struct qd_response_packet *response){
	//TODO
}
int qd_send_discover(int qdfd, const struct qd_discover_packet *discover){
	//TODO
}
int qd_client_socket(){
	//TODO
	getaddrinfo();
	int fd = socket(address_info->ai_family,address_info->ai_socktype,0);
	return fd;
}

//============ server use ============
//------ low level ------
int qd_send_response(int qdfd, const struct qd_response_packet *response, const struct sockaddr *address, socklen_t address_len){
	//TODO
}
int qd_recv_discover(int qdfd, struct qd_discover_packet *discover, struct sockaddr *address, socklen_t *address_len){
	//TODO
}
int qd_server_socket(){
	//TODO
}
//get address of the server
static int _get_machine_address(struct sockaddr *addr, socklen_t *addrlen){
	struct ifaddrs *addrs;
	int result = getifaddrs(&addrs);
	if (result < 0) return -1;
	for (;addrs != NULL; addrs = addrs.ifa_next){
		int family = addrs->ifa_addr->sa_family;
		if (family == AF_INET || family == AF_INET6){
			socklen_t address_len = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)
			*addrlen = address_len;
			memcpy(addr,&addrs->ifa_addr,address_len);
			freeifaddrs(addrs);
			return 0;
		}
	}
	freeifaddrs(addrs);
	return -1;
}
//called by the server thread
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
		arg->init_status = errno;
		return;
	}
	uint8_t hostname_len = strlen(hostname);
	//get a socket
	FD_SCOPED socket = qd_server_socket();
	if (socket < 0){
		arg->init_status = errno;
		return;
	}
	//signal setup was successful
	pthread_mutex_lock(&arg->init_event_condition);
	pthread_cond_broadcast(&arg->init_event_condition);
	pthread_mutex_unlock(&arg->init_event_condition);
	arg->init_status = 0;
	//==== listening loop ====
	for (;;){
		//listen for discover requests
		struct qd_discover_packet discover_packet;
		struct sockaddr sender_address = {0};
		socklen_t sender_address_len = 0;
		int result = qd_recv_discover(socket,&discover_packet,&sender_address,&sender_address_len);
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
		//get address
		struct sockaddr_storage address = {0};
		socklen_t address_len;
		result = _get_machine_address(&address,&address_len);
		if (result != 0){
			continue;
		}
		//reply
		struct qd_response_packet response_packet = {0}
		response_packet.hostname_len = hostname_len;
		memcpy(&response_packet.hostname,hostname,hostname_len);
		memcpy(&response_packet.address,address,address_len);
		response_packet.address_len = address_len;
		memcpy(&response_packet.service,SERVICE_LEN);
		result = qd_send_response(&response_packet,&sender_address,sender_address_len);
		if (result != 0){
			continue; //useless now but stops me forgetting if i change later
		}
	}
}
//------ high level ------
//spawns a thread that advertises the given service
//returns 0 on failure and > 0 on success
pthread_t start_qd_server(const char *service){
	struct _qd_server_thread_arg arg = {
		.init_event_mutex = PTHREAD_MUTEX_INITIALIZER,
		.init_event_condition = PTHREAD_COND_INITIALIZER,
	};
	memcpy(&arg.service,service,SERVICE_LEN);
	/*by locking this here the thread cant signal it is done before we start waiting
	for it as it has to wait for the mutex to unlock which happens atomicaly
	when pthread_cond_wait it called*/
	pthread_mutex_lock(&arg.init_event_mutex);
	//create new thread
	pthread_t thread;
	int result = pthread_create(&thread,NULL,&_qd_server_thread,service_copy);
	//wait untill thread has initialised
	pthread_cond_wait(&arg.init_event_condition,&arg.init_event_mutex);
	pthread_mutex_destroy(&arg.init_event_condition);
	//check initialisation status
	if (arg.init_status > 0){
		errno = arg.init_status;
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
