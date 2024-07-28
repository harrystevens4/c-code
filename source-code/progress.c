#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include "args.h"
#include <unistd.h>

int total_lines;
struct winsize window_size;
int width;

void print_line(const char *line);
void print_progress(int progress, int total);
int main(int argc, char **argv){
	ioctl(STDOUT_FILENO,TIOCGWINSZ, &window_size);
	width = window_size.ws_col;
	total_lines = 3;
	struct args args;
	parse_args(argc,argv,&args);
	free_args(&args);
	print_line("a");
	sleep(1);
	print_line("b");
	sleep(1);
	print_line("c");
	sleep(1);
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
