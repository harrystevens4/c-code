#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define READ_CHUNK_SIZE 10
void print_help();
int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr,"incorrect number of arguments. Try --help\n");
		return 1;
	}
	if (strcmp(argv[1],"--help") == 0){
		print_help();
		return 0;
	}
	int max_line_length = strtol(argv[1],NULL,10);
	if (max_line_length == 0){
		fprintf(stderr,"please specify a valaid max line length\n");
	}
	char *buffer = NULL;
	for (int i = 0;!feof(stdin);i++){
		buffer = realloc(buffer,sizeof(char)*((i+1)*READ_CHUNK_SIZE+1));
		fread(buffer+(i*READ_CHUNK_SIZE),1,READ_CHUNK_SIZE,stdin);
	}
	if (strlen(buffer) > max_line_length){
		buffer[max_line_length] = '\n';
		buffer[max_line_length+1] = '\0';
	}
	printf("%s",buffer);
	free(buffer);
}
void print_help(){
	printf("Used to cut off a line piped to it so that it is n chars long, and makes sure it ends with a newline\n");
	printf("Usage:\n");
	printf("	clamp-line-length <max line length> : clamps the line piped to it to <max line length>\n");
}
