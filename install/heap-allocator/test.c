#include "my-heap-allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *abrt_null(void *ptr) {
	if (ptr == NULL) abort();
	return ptr;
}

int main(){
	printf("============ malloc and free ============\n");
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
	printf("============ realloc ============\n");
	printf("allocating...\n");
	char *a1 = mha_alloc(50);
	char *a2 = mha_alloc(40);
	char *a3 = mha_alloc(30);
	char *a4 = mha_alloc(20);
	mha_free(a3);
	a3 = NULL;
	print_heap_state();
	printf("reallocating chunk 2...\n");
	a2 = abrt_null(mha_realloc(a2,60));
	print_heap_state();
	printf("allocating...\n");
	char *a5 = mha_alloc(100);
	print_heap_state();
	printf("reallocating last chunk...\n");
	a5 = mha_realloc(a5,200);
	print_heap_state();
	printf("reallocating last chunk again...\n");
	a5 = mha_realloc(a5,50);
	print_heap_state();
	return 0;
}
