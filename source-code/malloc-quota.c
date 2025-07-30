#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *(*_malloc)(size_t size);
static void (*_free)(void *);
static void *(*_realloc)(void *,size_t size);
static void *(*_calloc)(size_t nmemb,size_t size);
static void init() __attribute((constructor));
static void cleanup();

struct {
	size_t total_size;
	size_t allocations;
	size_t frees;
	size_t final_size;
	size_t failed_allocations;
} info;

struct allocation_info {
	size_t size;
};
typedef struct allocation_info allocation_t;

// +internaly malloc and free from here (allocation is sizeof(allocation_t) bytes bigger)
// |           +pointer returned points here
// |           |
// 00 00 00 04 00 00 00 00
// ^^^^^^^^^^^ ^^^^^^^^^^^
// struct      actual data allocated and pointed to
// holding
// size of
// block

void *malloc(size_t size){
	void *result = _malloc(size+sizeof(allocation_t));
	if (result != NULL){
		info.total_size += size;
		info.allocations++;
		info.final_size += size;
		//store metadata on the heap
		allocation_t *metadata = result;
		memset(metadata,0,sizeof(allocation_t));
		metadata->size = size;
	}else{
		info.failed_allocations += 1;
	}
	return (char *)result+sizeof(allocation_t);
}

void free(void *pointer){
	if (pointer == NULL) return;
	allocation_t *metadata = pointer-sizeof(allocation_t);
	info.final_size -= metadata->size;
	info.frees++;
	_free((char *)pointer-sizeof(allocation_t));
}

static void init(){
	atexit(cleanup);
	_malloc = dlsym(RTLD_NEXT,"malloc");
	_free = dlsym(RTLD_NEXT,"free");
	_realloc = dlsym(RTLD_NEXT,"realloc");
	_calloc = dlsym(RTLD_NEXT,"calloc");
	memset(&info,0,sizeof(info));
}

void *realloc(void *pointer,size_t new_size){
	void *result; 
	if (pointer == NULL){
		result = _realloc(NULL,new_size+sizeof(allocation_t));
	}else{
		result = _realloc((char *)pointer-sizeof(allocation_t),new_size+sizeof(allocation_t));
	}
	if (result != NULL){
		if (pointer == NULL) memset(result,0,sizeof(allocation_t));
		allocation_t *metadata = result;
		info.final_size += (long long int)new_size - (long long int)metadata->size;
		metadata->size = new_size;
	}else{
		info.failed_allocations++;
	}
	return result+sizeof(allocation_t);
}
void *calloc(size_t nmemb, size_t size){
	size_t chunk_size = nmemb*size;
	//detect overflow
	if (nmemb != 0 && chunk_size / nmemb != size){
		info.failed_allocations++;
		return NULL;
	}
	return malloc(chunk_size);
}

void *reallocarray(void *pointer,size_t nmemb,size_t size){
	fprintf(stderr,"reallocarray not implemented");
	abort();
}

static void cleanup(){
	printf("====== malloc-quota ======\n");
	printf("total bytes allocated: %lu\n",info.total_size);
	printf("total allocations: %lu\n",info.allocations);
	printf("total frees: %lu\n",info.frees);
	printf("final size of heap: %lu\n",info.final_size);
	printf("failed allocations: %lu\n",info.failed_allocations);
}

int main(){
	void *p = malloc(200);
	free(p);
	void *q = calloc(8,20);
	free(q);
}
