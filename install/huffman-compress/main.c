#include "../../source-code/huffman.h"
#include <getopt.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#define READ_CHUNK_SIZE 1024

void print_help(char *name){
	printf("usage: %s [options]\n",name);
	printf("options:\n");
	printf("	-i, --input-file  : input file, defaults to stdin\n");
	printf("	-o, --output-file : output file, defaults to stdout\n");
	printf("	-c, --compress    : compress input file to output file (default)\n");
	printf("	-d, --decompress  : decompress input file to output file\n");
}

int main(int argc, char **argv){
	FILE *input_file = stdin;
	FILE *output_file = stdout;
	int option_index = 0;
	enum {COMPRESS_MODE = 0,DECOMPRESS_MODE = 1} mode = 0;
	//====== get program arguments ======
	struct option long_options[] = {
		{"help",no_argument,0,'h'},
		{"compress",no_argument,0,'c'},
		{"decompress",no_argument,0,'d'},
		{"input-file",required_argument,0,'i'},
		{"output-file",required_argument,0,'o'},
		{0,0,0,0}
	};
	for (;;){
		int result = getopt_long(argc,argv,"hcdi:o:",long_options,&option_index);
		if (result == -1) break;
		switch (result){
		case 'h':
			print_help(argv[0]);
			return 0;
		case 'd':
			mode = DECOMPRESS_MODE;
			break;
		case 'c':
			mode = COMPRESS_MODE;
			break;
		case 'i':
			input_file = fopen(optarg,"rb");
			if (input_file == NULL){
				fprintf(stderr,"Error opening %s\n",optarg);
				perror("fopen");
				return -1;
			}
			break;	
		case 'o':
			output_file = fopen(optarg,"wb");
			if (output_file == NULL){
				fprintf(stderr,"Error opening %s\n",optarg);
				perror("fopen");
				return -1;
			}
			break;	
		default:
			fprintf(stderr,"Unknown argument %c\n",result);
			break;
		}
	}
	if (optind < argc){
		fprintf(stderr,"Too many non option arguments.\n");
		return -1;
	}
	//====== read the input file ======
	char *processed_data = NULL;
	size_t input_buffer_size = 0;
	char *input_buffer = NULL;
	for (;;){
		input_buffer = realloc(input_buffer,input_buffer_size+READ_CHUNK_SIZE);
		size_t size = fread(input_buffer+input_buffer_size,1,READ_CHUNK_SIZE,input_file);
		input_buffer_size += size;
		if (size < READ_CHUNK_SIZE){
			if (ferror(input_file)){
				fprintf(stderr,"Error occured while reading input file\n");
				perror("fread");
				goto cleanup;
			}
			break;
		}
	}
	//====== do the compression / decompression ======
	size_t processed_size = 0;
	if (mode == COMPRESS_MODE) processed_size = hfmn_compress(input_buffer,input_buffer_size,&processed_data);
	if (mode == DECOMPRESS_MODE) processed_size = hfmn_decompress(input_buffer,input_buffer_size,&processed_data);
	//====== write to output file ======
	size_t size = fwrite(processed_data,1,processed_size,output_file);
	if (size < processed_size){
		fprintf(stderr,"Error occured while writing to output file\n");
		perror("fwrite");
		goto cleanup;
	}
	//====== cleanup ======
	cleanup:
	free(processed_data);
	free(input_buffer);
	fclose(input_file);
	fclose(output_file);
	return 0;
}
