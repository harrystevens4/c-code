#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include "args.h"
#include "shell-toolkit.h"
#include <unistd.h>
#define BUFFER_SIZE 1024

int total_lines;
struct winsize window_size;
int width;
int line_count;

void print_line(const char *line);
void print_progress(int progress, int total);
int main(int argc, char **argv){
	int autodetected = 0;
	line_count = 0;
	int cache_exists = 0;
	int buffer_size = BUFFER_SIZE;
	char *data;
	FILE *script;
	FILE *script_cache;
	char script_cache_location[BUFFER_SIZE];
	char buffer[BUFFER_SIZE];
	char script_location[BUFFER_SIZE];
	char command_buffer[BUFFER_SIZE];
	int pid = 660;
	char executable_location[BUFFER_SIZE];
	char *result;
	FILE *output_file;
	ioctl(STDOUT_FILENO,TIOCGWINSZ, &window_size);
	width = window_size.ws_col;
	total_lines = 0;
	struct args args;
	parse_args(argc,argv,&args);
	if ((args.number_single > 0) && (args.number_other > 0)){
		if (args.single[0] == 'c'){
			total_lines = (int)strtol(args.other[0],(char **)NULL, 10);
		}

	}else{
		//printf("attempting analasys of bash script\n");
		get_pipe_input_bash_script_location(script_location,BUFFER_SIZE);
		//printf("%s\n",script_location);
		snprintf(script_cache_location,BUFFER_SIZE,"%s.progress",script_location);
		//printf("attempting to open %s\n",script_cache_location);
		if (script_cache = fopen(script_cache_location,"r")){// if a cache for the script already exists
			fgets(buffer,BUFFER_SIZE,script_cache);	
			total_lines = (int)strtol(buffer,(char**)NULL, 10);
			autodetected = 1;
			cache_exists = 1;
			fclose(script_cache);
		}else{
			//analasys
			//printf("could not open cache, attempting to guess total lines\n");
			//count the number of lines that contain "echo" or "printf"
			script = fopen(script_location,"r");
			for (;;){
				if (feof(script)){
					break; //stop at end of file
				}
				fgets(buffer,BUFFER_SIZE,script);
				if(strstr(buffer,"echo")){
					total_lines++;
				}
			}
			fclose(script);
			//printf("analysed with result of %d lines detected\n",total_lines);
			autodetected = 1;
		}
	}
	for (;;){
		if (((args.number_other == 0) && (args.number_single == 0)) && (autodetected == 0)){
			total_lines++;
		}
		data = fgets(buffer, sizeof(buffer), stdin);
		if (data == NULL){// if the pipe is closed, stop
			break;
		}
		if (buffer[strlen(buffer)-1] == '\n'){
			buffer[strlen(buffer)-1] = '\0';
		}
		print_line((const char *)buffer);
	}
	if (!(cache_exists)){
		//printf("updating cache...\n");
		script_cache = fopen(script_cache_location,"w");
		fprintf(script_cache,"%d",line_count);
		fclose(script_cache);
		//printf("done!\n");
	}
	free_args(&args);
	printf("\n"); //make sure to clean up after progress bar
	return 0;
}
void print_line(const char *line){
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
