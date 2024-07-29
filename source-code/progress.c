#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include "args.h"
#include <unistd.h>
#define BUFFER_SIZE 1024

int total_lines;
struct winsize window_size;
int width;

void print_line(const char *line);
void print_progress(int progress, int total);
int main(int argc, char **argv){
	int buffer_size = BUFFER_SIZE;
	char *data;
	char buffer[buffer_size];
	ioctl(STDOUT_FILENO,TIOCGWINSZ, &window_size);
	width = window_size.ws_col;
	total_lines = 0;
	struct args args;
	parse_args(argc,argv,&args);
	for (;;){
		if ((args.number_other == 0) && (args.number_single == 0)){
			total_lines++;
		}
		data = fgets(buffer, sizeof(buffer), stdin);
		if (data == NULL){// if the pipe is closed, stop
			return 0;
		}
		if (buffer[strlen(buffer)-1] == '\n'){
			buffer[strlen(buffer)-1] = '\0';
		}
		print_line((const char *)buffer);
	}
	free_args(&args);
	printf("\n"); //make sure to clean up after progress bar
	return 0;
}
void print_line(const char *line){
	static int line_count = 0;
	line_count++;
	printf("\033[2K\r"); //erase existing progress bar
	printf("%s\n",line);
	//create new progress bar
	print_progress(line_count,total_lines);
}
void print_progress(int progress, int total){
	//(index/total)[####      ]
	char *buffer;
	int counter_length = snprintf(buffer,0,"(%d/%d)",progress,total,width);//get length of counter part of progress info
	int progress_width = width-counter_length-2; // acount for the "[" and "]"
	int progress_fill_count = (((float)progress/(float)total)*progress_width);

	printf("(%d/%d)",progress,total,width);
	
	printf("[");
	for (int i = 0;i<progress_width;i++){
		if (i<=progress_fill_count){
			printf("#");
		}else{
			printf(" ");
		}
	}
	printf("]");
	printf("\n\033[1A");//newline to push buffer to stdout and up controll char to move cursor back
}
