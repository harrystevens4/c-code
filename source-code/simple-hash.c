#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
int main(int argc , char** argv){
	if (argc != 2){
		fprintf(stderr,"please provide 1 argument to be hashed\n");
		return 1;
	}

	char *input_string = argv[1]; //arg 1
	
	//setup an empty uint64_t array to cast into
	int int_buffer_length = 0;//number of elements
	for (;int_buffer_length*sizeof(uint64_t) < strlen(input_string)*sizeof(char);){
		int_buffer_length++;
	}
	uint64_t *int_buffer;
	//char buffer to ensure we own the memory
	int buffer_size = sizeof(uint64_t)*int_buffer_length;
	char *buffer = malloc(buffer_size);
	memset(buffer,0,buffer_size);
	strncpy(buffer,input_string,buffer_size);

	//length extention
	if (strlen(input_string)*sizeof(char) < sizeof(uint64_t)){
		for (int i = strlen(input_string)-1; i < sizeof(uint64_t)-1;i++){
			buffer[i+1] = buffer[i];
		}
	}

	int_buffer  = (uint64_t *)buffer; //cast string to an array of 64 bit ints
	uint64_t final_hash = 0;
	for (int i = 0; i < int_buffer_length; i++){
		final_hash += int_buffer[i];
	}
	printf("%lu\n",final_hash);
	free(buffer); //also frees int_buffer due to being the same pointer
	return 0;
}
