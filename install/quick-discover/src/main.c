#include "../../../source-code/quick-discover.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static void empty_signal_handler(int _){}

int main(int argc, char **argv){
	//====== handle arguments ======
	if (argc < 2){
		fprintf(stderr,"Please provide the name of the service\n");
		return -1;
	}
	char service_name[QD_SERVICE_LEN] = {0};
	strncpy(service_name,argv[1],QD_SERVICE_LEN);
	//====== start service ======
	printf("starting service...\n");
	pthread_t service = start_qd_server(service_name);
	if (service == 0){
		perror("start_qd_server");
		return -1;
	}
	printf("service started\n");
	//====== do nothing professionaly ======
	struct sigaction old_sigint_action = {0};
	struct sigaction new_sigint_action = {
		.sa_handler = &empty_signal_handler,
	};
	//save
	sigaction(SIGINT,&new_sigint_action,&old_sigint_action);
	//wait
	pause();
	//restore
	sigaction(SIGINT,&old_sigint_action,NULL);
	//====== stop the service when we get bored of doing nothing ======
	printf("stopping service...\n");
	stop_qd_server(service);
	printf("stopped\n");
	return 0;
}
