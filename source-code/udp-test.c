#include <stdio.h>
#include "tcp-toolkit.h"
int main(int argc, char **argv){
	verbose_tcp_toolkit = 1;
	silent_errors = 0;
	if (argc > 1){
		printf("returned %d\n",broadcast_existence("8500"));
	}else{
		find_broadcasters("8500");
	}
}
