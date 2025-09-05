#include "../../source-code/huffman.h"
#include <assert.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

int main(int argc, char **argv){
	//printf("%d\n",UCHAR_MAX+1);
	////====== open the file ======
	//int input_fd = open(argv[2],O_RDONLY);
	//if (input_fd != 0){
	//	perror("open");
	//	return 1;
	//}
	//struct stat statbuf;
	//int result = fstat(input_fd,&statbuf);
	//if (result != 0){
	//	perror("fstat");
	//	return 1;
	//}
	////======= read the file ======
	//size_t buffer_size = statbuf.st_size;
	//char *buffer = malloc(buffer_size);
	//result = read(input_fd,buffer,buffer_size);
	//if (result != 0){
	//	perror("read");
	//	return 1;
	//}
	char *buffer = "the quick brown fox jumped over the lazy dog";
	size_t buffer_size = strlen(buffer)+1;
	char *compressed_data;
	char *decompressed_data;
	size_t compressed_size = hfmn_compress(buffer,buffer_size,&compressed_data);
	size_t decompressed_size = hfmn_decompress(compressed_data,compressed_size,&decompressed_data);
	assert(buffer_size == decompressed_size);
	assert(strcmp(buffer,decompressed_data) == 0);
	printf("%s\n",decompressed_data);
	printf("decompressed size: %lu\n",buffer_size);
	printf("compressed size: %lu\n",compressed_size);
	free(compressed_data);
	free(decompressed_data);
	compressed_size = hfmn_compress(NULL,0,&compressed_data);
	hfmn_decompress(compressed_data,compressed_size,&decompressed_data);
	free(compressed_data);
}
