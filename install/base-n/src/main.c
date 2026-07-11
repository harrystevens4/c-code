#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include "../../../source-code/base_n.h"

void print_help();

int main(int argc, char **argv){
	//====== handle arguments ======
	if (argc < 2){
		fprintf(stderr,"Please provide a command. See \"%s help\" for help\n",argv[0]);
		return EXIT_FAILURE;
	}
	if (strcmp(argv[1],"help") == 0){
		print_help();
		return EXIT_SUCCESS;
	}
	if (argc < 3){
		fprintf(stderr,"Please provide data. See \"%s help\" for help\n",argv[0]);
		return EXIT_FAILURE;
	}
	//====== decode ======
	char *input = argv[2];
	size_t input_len = strlen(argv[2]);
	//validate
	if (input_len % 4 != 0){
		fprintf(stderr,"Invalid input\n");
		return EXIT_FAILURE;
	}
	//im using alloca because it seems cool
	ssize_t output_len = (input_len*6) / 8;
	char *output = alloca(output_len);
	memset(output,0,output_len);
	output_len = base64_decode(input,input_len,output);
	if (output_len < 0){
		fprintf(stderr,"Could not decode\n");
		return EXIT_FAILURE;
	}
	printf("%.*s\n",(int)output_len,output);
	return EXIT_SUCCESS;
}

void print_help(){
	printf("usage: base-n <command> <data>\n");
	printf("command is either encode or decode\n");
}

