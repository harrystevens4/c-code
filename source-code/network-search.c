#include <stdio.h>
#include "tcp-toolkit.h"
#include "args.h"
void print_help(){
	printf("Usage:\n");
	printf("	network-search [options] [port]\n");
	printf("	-b : broadcast\n");
	printf("	-f : find active broadcasts\n");
	printf("Return values:\n");
	printf("	error : exit status 1\n");
	printf("	success : ip address of found broadcast\n");
}
int main(int argc, char **argv){
	verbose_tcp_toolkit = 1;
	silent_errors = 0;
	if (argc < 2){
		print_help();
		return 1;
	}
	struct args args;
	parse_args(argc,argv,&args);
	if (args.number_other != 1 || args.number_single != 1){
		print_help();
		return 1;
	}
	if (args.single[0] == 'b'){
		broadcast_existence(args.other[0]);
	}else if (args.single[0] == 'f'){
		printf("%s\n",find_broadcasters(args.other[0]));
	}else{
		print_help();
		return 1;
	}
	free_args(&args);
	
}
