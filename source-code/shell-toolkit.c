#include "shell-toolkit.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

static volatile int output_tty; // 0 for no 1 for yes
static volatile int input_tty; // 0 for no 1 for yes
static char return_buffer[1024];

int connected_to_tty(){//returns 1 if both stdout and stdin are a tty
	output_tty = 0;
	input_tty = 0;
	if (isatty(STDOUT_FILENO)){
		output_tty = 1;
	}
	if (isatty(STDIN_FILENO)){
		input_tty = 1;
	}
	if (output_tty && input_tty){
		return 1;
	}else{
		return 0;
	}
}
char *get_calling_process_executable(){
	return_buffer[0] == '\0';
	char command_buffer[1024];
	int pid = 660;
	char *result;
	FILE *output_file;
	//check if connected to pipe
	if (connected_pipe_output()){//detect if we should get other side of pipe or parent pid
		pid = getpgrp();//magic command to get the other side of the pipes pid             
		printf("using parent group id\n");
	}else{
		printf("using parent pid\n");
		pid = getppid();
	}
	//attempt to find the executable location corresponding to the pid
	sprintf(command_buffer,"readlink -f /proc/%d/exe",pid);//get executable location   
	output_file = popen(command_buffer,"r");                                           
	result = fgets(return_buffer,1024,output_file);                              
	pclose(output_file); 
	//stripping newlines from the end
	if (return_buffer[strlen(return_buffer)-1] == '\n'){
		return_buffer[strlen(return_buffer)-1] = '\0';
	}
	return return_buffer;
}
int connected_pipe_input(){//returns 1 on true (used in if statement)
	connected_to_tty();
	return !output_tty;
}
int connected_pipe_output(){//returns 1 on true
	connected_to_tty();//sets the output_tty and input_tty variables
	return !input_tty;
}
