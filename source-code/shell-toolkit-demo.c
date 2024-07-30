#include <stdio.h>
#include "shell-toolkit.h"
int main(){
	printf("pipe output connected:%d\npipe input connected:%d\n",connected_pipe_output(),connected_pipe_input());
	printf("called by:%s\n",get_calling_process_executable());
	return 0;
}
