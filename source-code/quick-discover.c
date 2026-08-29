//Program for quickly discovering a device with specific hostname on the network
//find documentation in header file
#include "quick-discover.h"
#include <ifaddrs.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <net/if.h>
#include <poll.h>

struct _qd_server_thread_arg {
	char service[QD_SERVICE_LEN];
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

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define FD_SCOPED \
	__attribute__((__cleanup__(_close_shim))) int

//all functions return 0 or > 0 on success and < 0 on error

//============ client use ============
//------ low level ------
int qd_recv_response(int qdfd, struct qd_response_packet *response){
	/*if we just go with what the sender sent us rather than
	what address we received from than it is much more flexible
	because we could have one server that advertises for all the other devices
	which would allow things like load balancing by providing different machine
	addresses on a round robin or something*/
	return recvfrom(qdfd,response,sizeof(*response),0,NULL,NULL);
}
int qd_send_discover(int qdfd, const struct qd_discover_packet *discover){
	//im just gonna put the ipv4 in the bag because i cba rn with ipv6
	const struct sockaddr_in broadcast_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(QD_PORT),
		.sin_addr = {
			.s_addr = 0xffffffff,
		},
	};
	socklen_t broadcast_addr_len = sizeof(struct sockaddr_in);
	return sendto(qdfd,discover,sizeof(*discover),0,(struct sockaddr *)&broadcast_addr,broadcast_addr_len);
}
int qd_client_socket(){
	int fd = socket(AF_INET,SOCK_DGRAM,0);
	if (fd < 0) return -1;
	const int enable = 1;
	int result = setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&enable,sizeof(int));
	if (result < 0){
		close(fd);
		return -1;
	}
	return fd;
}

static unsigned long _monotonic_time_ms(){
	struct timespec result;
	clock_gettime(CLOCK_MONOTONIC,&result);
	return result.tv_sec*1000 + result.tv_nsec/1000000
}

//------ high level ------
//service and hostname can both be NULL
//service is also a regular string here it will be truncated if needed for you
//hostname is also a regular string
//use qd_response_free() when you are done
struct qd_response *qd_discover(const char *service, const char *hostname, int timeout_ms){
	//====== prep discover request ======
	struct qd_discover_packet discover_packet = {0};
	if (service != NULL){
		strncpy(discover_packet.service,service,QD_SERVICE_LEN);
	}
	if (hostname != NULL){
		//discover_packet.service is HOSTNAME_MAX_LEN+1 so it will always have a
		//null byte chilling at the end
		strncpy(discover_packet.hostname,hostname,HOSTNAME_MAX_LEN);
		discover_packet.hostname_len = MIN(strlen(hostname),HOSTNAME_MAX_LEN);
	}
	//====== setup socket ======
	FD_SCOPED qdfd = qd_client_socket();
	if (qdfd < 0) return NULL;
	//====== send discover request ======
	int result = qd_send_discover(qdfd,&discover_packet);
	if (result < 0){
		return NULL;
	}
	//===== receiving loop ======
	struct qd_response *responses = NULL;
	struct qd_response *current_response = NULL;
	for (;;){
		//====== check stuff is available ======
		struct pollfd poll_fds[1] = {{
			.fd = qdfd,
			.events = POLLIN,
		}};
		long poll_start_time = _monotonic_time_ms();
		int result = poll(poll_fds,1,timeout_ms);
		if (result < 0){
			//error
			qd_response_free(responses);
			return NULL;
		}
		if (result == 0){
			//no fd available (timeout exipred)
			break;
		}
		//fd available
		timeout_ms -= _monotonic_time_ms() - poll_start_time;
		//====== receive a response ======
		struct qd_response_packet response_packet = {0};
		result = qd_recv_response(qdfd,&response_packet);
		if (result < 0){
			qd_response_free(responses);
			return NULL;
		}
		//====== read data into response ======
		//allocate node
		if (current_response == NULL){
			responses =malloc(sizeof(struct qd_response)); 
			current_response = responses;
		}else{
			current_response->next = malloc(sizeof(struct qd_response));
			current_response = current_response->next;
		}
		//address
		if (response_packet.is_ipv4){
			current_response->addr = malloc(sizeof(struct sockaddr_in));
			struct sockaddr_in *in_addr = (struct sockaddr_in *)current_response->addr;
			in_addr->sin_addr = response_packet.address.inet;
			current_response->addrlen = sizeof(struct sockaddr_in);
		}else {
			struct sockaddr_in6 *in6_addr = malloc(sizoef(struct sockaddr_in6));
			current_response->addr = (struct sockaddr *)in6_addr;
			current_response->addrlen = sizeof(struct sockaddr_in6);
			memcpy(sin6_addr->sin6_addr,response_packet.address.inet6);
		}
		//hostname
		memset(current_response->hostname,0,sizeof(current_response->hostname));
		strncpy(current_response->hostname,response_packet.hostname,MIN(response_packet.hostname_len,HOSTNAME_MAX_LEN));
		//service
		strncpy(current_response->service,response_packet.service,QD_SERVICE_LEN);
	}
	//====== cleanup ======
	return responses;
}

//TODO
void qd_response_free(struct qd_response *response){
	if (response == NULL) return;
}

//============ server use ============
//------ low level ------
int qd_send_response(int qdfd, const struct qd_response_packet *response, const struct sockaddr *address, socklen_t address_len){
	return sendto(qdfd,response,sizeof(*response),0,address,address_len);
}
int qd_recv_discover(int qdfd, struct qd_discover_packet *discover, struct sockaddr *address, socklen_t *address_len){
	return recvfrom(qdfd,discover,sizeof(*discover),0,address,address_len);
}
int qd_server_socket(){
	struct addrinfo *address_info, hints = {
		.ai_socktype = SOCK_DGRAM,
		.ai_family = AF_UNSPEC,
		.ai_flags = AI_PASSIVE,
	};
	errno = ENOTSUP; //set incase gai error not an errno error
	int result = getaddrinfo(NULL,QD_PORT_STRING,&hints,&address_info);
	if (result < 0) return -1;
	int fd = socket(address_info->ai_family,address_info->ai_socktype,0);
	if (fd < 0){
		freeaddrinfo(address_info);
		return -1;
	}
	const int enable = 1;
	result = setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(int));
	if (result < 0){
		close(fd);
		return -1;
	}
	result = setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,&enable,sizeof(int));
	if (result < 0){
		close(fd);
		return -1;
	}
	result = bind(fd,address_info->ai_addr,address_info->ai_addrlen);
	freeaddrinfo(address_info);
	if (result < 0){
		close(fd);
		return -1;
	}
	return fd;
}
//get address of the server
static int _get_machine_address(struct sockaddr *addr, socklen_t *addrlen){
	struct ifaddrs *addrs;
	int result = getifaddrs(&addrs);
	if (result < 0) return -1;
	for (struct ifaddrs *current_addr = addrs; current_addr != NULL; current_addr = current_addr->ifa_next){
		int family = current_addr->ifa_addr->sa_family;
		if (current_addr->ifa_flags & IFF_LOOPBACK) continue;
		if (family == AF_INET || family == AF_INET6){
			socklen_t address_len = (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
			*addrlen = address_len;
			memcpy(addr,current_addr->ifa_addr,address_len);
			freeifaddrs(addrs);
			return 0;
		}
	}
	freeifaddrs(addrs);
	return -1;
}
//called by the server thread
static void *_qd_server_thread(void *_qd_server_thread_arg){
	//==== setup ====
	struct _qd_server_thread_arg *arg = _qd_server_thread_arg;
	//extract the service
	char service[QD_SERVICE_LEN] = {0};
	memcpy(service,&arg->service,QD_SERVICE_LEN);
	//grab hostname
	char hostname[HOSTNAME_MAX_LEN+1] = {0};
	int result = gethostname(hostname,HOSTNAME_MAX_LEN);
	if (result < 0){
		arg->init_status = errno;
		return NULL;
	}
	uint8_t hostname_len = strlen(hostname);
	//get a socket
	FD_SCOPED socket = qd_server_socket();
	if (socket < 0){
		arg->init_status = errno;
		return NULL;
	}
	//signal setup was successful
	pthread_mutex_lock(&arg->init_event_mutex);
	pthread_cond_broadcast(&arg->init_event_condition);
	pthread_mutex_unlock(&arg->init_event_mutex);
	arg->init_status = 0;
	//==== listening loop ====
	for (;;){
		//listen for discover requests
		struct qd_discover_packet discover_packet = {0};
		struct sockaddr_storage sender_address = {0};
		socklen_t sender_address_len = sizeof(sender_address);
		int result = qd_recv_discover(socket,&discover_packet,(struct sockaddr *)&sender_address,&sender_address_len);
		if (result < 0){
			perror("qd_recv_discover");
			continue;
		}
		//filter any non relevant requests
		if (discover_packet.hostname_len != 0 ){
			if (strncmp(hostname,discover_packet.hostname,discover_packet.hostname_len) != 0) continue;
		}
		if (discover_packet.service[0] != '\0'){
			if (strncmp(discover_packet.service,service,QD_SERVICE_LEN) != 0) continue;
		}
		//get address
		struct sockaddr_storage address = {0};
		socklen_t address_len = 0;
		result = _get_machine_address((struct sockaddr *)&address,&address_len);
		if (result != 0){
			continue;
		}
		//reply
		struct qd_response_packet response_packet = {0};
		response_packet.hostname_len = hostname_len;
		memcpy(&response_packet.hostname,hostname,hostname_len);
		//fill in the address
		if (address.ss_family == AF_INET){
			struct sockaddr_in *in_addr = (struct sockaddr_in *)&address;
			memcpy(&response_packet.address.inet,&in_addr->sin_addr,sizeof(struct in_addr));
		}else if (address.ss_family == AF_INET6){
			struct sockaddr_in6 *in6_addr = (struct sockaddr_in6 *)&address;
			memcpy(&response_packet.address.inet6,&in6_addr->sin6_addr,sizeof(struct in6_addr));
		}
		response_packet.is_ipv4 = (address.ss_family == AF_INET) ? 1 : 0;
		memcpy(&response_packet.service,service,QD_SERVICE_LEN);
		result = qd_send_response(socket,&response_packet,(struct sockaddr *)&sender_address,sender_address_len);
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
	memcpy(&arg.service,service,QD_SERVICE_LEN);
	/*by locking this here the thread cant signal it is done before we start waiting
	for it as it has to wait for the mutex to unlock which happens atomicaly
	when pthread_cond_wait it called*/
	pthread_mutex_lock(&arg.init_event_mutex);
	//create new thread
	pthread_t thread;
	int result = pthread_create(&thread,NULL,&_qd_server_thread,&arg);
	//wait untill thread has initialised
	pthread_cond_wait(&arg.init_event_condition,&arg.init_event_mutex);
	pthread_mutex_destroy(&arg.init_event_mutex);
	//check initialisation status
	if (result < 0) return -1;
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
	pthread_join(server_thread,NULL);
}
