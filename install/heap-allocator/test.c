#include "my-heap-allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	printf("initialising...\n");
	mha_free(NULL);
	print_heap_state();
	printf("allocating...\n");
	char *string_1 = mha_alloc(50);
	print_heap_state();
	printf("allocating...\n");
	char *string_2 = mha_alloc(7);
	char *string_3 = mha_alloc(8);
	memcpy(string_1,"12345",6);
	printf("%s\n",string_1);
	print_heap_state();
	printf("freeing...\n");
	mha_free(string_1);
	print_heap_state();
	printf("allocating...\n");
	string_1 = mha_alloc(10);
	print_heap_state();
	printf("freeing...\n");
	mha_free(string_1);
	print_heap_state();
	printf("freeing...\n");
	mha_free(string_3);
	print_heap_state();
	printf("freeing...\n");
	mha_free(string_2);
	print_heap_state();
	return 0;
}
