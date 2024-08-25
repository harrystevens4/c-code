#include <stdio.h>
#include "tcp-toolkit.h"
#include "args.h"
void print_help(){
	printf("Usage:\n");
	printf("	network-search [options] [port]\n");
	printf("	-b : broadcast\n");
	printf("	-f : find active broadcasts\n");
	printf("	-t : set 5 second timeout (only works for -f)\n");
	printf("Return values:\n");
	printf("	error : exit status 1\n");
	printf("	success : ip address of found broadcast\n");
}
int main(int argc, char **argv){
	verbose_tcp_toolkit = 0;
	silent_errors = 0;
	if (argc < 2){
		print_help();
		return 1;
	}
	struct args args;
	parse_args(argc,argv,&args);
	if (args.number_other < 1 || args.number_single < 1){
		print_help();
		return 1;
	}
	for (int i =0;i<args.number_single;i++){
	switch (args.single[i]){
		case 'b':
		broadcast_existence(args.other[0]);
		break;
		case 'f':
		char * ip_string = find_broadcasters(args.other[0]);
		if (ip_string == NULL) return 1;
		printf("%s\n",ip_string);
		break;
		case 't':
		timeout = 5;
		break;
		default:
		print_help();
		return 1;
	}}
	free_args(&args);
	
}
