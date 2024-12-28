#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
int get_pedal_state(int pedal_fd);
int main(int argc, char **argv){
	char *device = "/dev/ttyACM0";
	int pedal_fd = open(device,O_RDONLY);
	if (pedal_fd < 0){
		perror("open");
		return 1;
	}
	for (;;){
		int pedal_state = get_pedal_state(pedal_fd);
		if (pedal_state < 0){
			fprintf(stderr,"main(): get_pedal_state error\n");
			close(pedal_fd);
			return 1;	
		}
		//lets not waste system resources
		//usleep(100000); //100,000us = 0.1s
		printf("%d\n",pedal_state);
	}
	close(pedal_fd);
	return 0;
}
int get_pedal_state(int pedal_fd){
	char state[2] = "0";

	int result = read(pedal_fd,&state,sizeof(char));
	if (result < 0){
		perror("read");
		return -1;
	}
	return strtol(state,NULL,10);
}
