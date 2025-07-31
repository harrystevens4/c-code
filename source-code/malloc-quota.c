#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void *(*_malloc)(size_t size);
static void (*_free)(void *);
static void *(*_realloc)(void *,size_t size);
static void *(*_calloc)(size_t nmemb,size_t size);
static void init() __attribute((constructor));
static void cleanup();
static char *format_bytes(long long int bytes);

#define MAX(x,y) (((x) > (y)) ? (x) : (y))

struct {
	size_t total_size;
	size_t allocations;
	size_t frees;
	size_t final_size;
	size_t failed_allocations;
	size_t max_allowance;
	size_t last_allocation_size;
} info =  {.max_allowance = -1};

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
	info.last_allocation_size = size;
	//dont overallocate
	if ((info.final_size + size) > info.max_allowance){
		fprintf(stderr,"=== allowance reached: %lu added to %lu ===\n",size,info.final_size);
		info.failed_allocations++;
		errno = ENOMEM;
		return NULL;
	}
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

void *realloc(void *pointer,size_t new_size){
	info.last_allocation_size = new_size;
	size_t old_size;
	if (pointer == NULL) old_size = 0;
	else old_size = ((allocation_t*)((char  *)pointer-sizeof(allocation_t)))->size;
	//dont overallocate
	size_t new_heap_size = (long long int)info.final_size + (long long int)new_size - (long long int)old_size;
	if (new_heap_size > info.max_allowance){
		fprintf(stderr,"=== allowance reached: %lu, old size: %lu ===\n",new_heap_size,old_size);
		info.failed_allocations++;
		errno = ENOMEM;
		return NULL;
	}
	void *result; 
	if (pointer == NULL){
		result = _realloc(NULL,new_size+sizeof(allocation_t));
		info.allocations++;
	}else{
		result = _realloc((char *)pointer-sizeof(allocation_t),new_size+sizeof(allocation_t));
	}
	if (result != NULL){
		if (pointer == NULL) memset(result,0,sizeof(allocation_t));
		allocation_t *metadata = result;
		info.final_size += (long long int)new_size - (long long int)metadata->size;
		info.total_size += MAX((long long int)new_size - (long long int)old_size,0);
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
	void *result = malloc(chunk_size);
	if (result == NULL) return NULL;
	memset((char *)result,0,chunk_size);
	return result;
}

void *reallocarray(void *pointer,size_t nmemb,size_t size){
	size_t chunk_size = nmemb*size;
	if (nmemb != 0 && chunk_size / nmemb != size){
		info.failed_allocations++;
		return NULL;
	}
	return realloc(pointer,chunk_size);
}

static void init(){
	atexit(cleanup);
	_malloc = dlsym(RTLD_NEXT,"malloc");
	_free = dlsym(RTLD_NEXT,"free");
	_realloc = dlsym(RTLD_NEXT,"realloc");
	_calloc = dlsym(RTLD_NEXT,"calloc");
	memset(&info,0,sizeof(info));
	char *allowance_string = getenv("MQ_ALLOWANCE");
	if (allowance_string == NULL) info.max_allowance = -1;
	else info.max_allowance = strtoll(allowance_string,NULL,10);

}

static void cleanup(){
	printf("====== malloc-quota ======\n");
	printf("total allowance: %s\n",format_bytes(info.max_allowance));
	printf("total bytes allocated: %s\n",format_bytes(info.total_size));
	printf("total allocations: %lu\n",info.allocations);
	printf("total frees: %lu\n",info.frees);
	printf("final size of heap: %s\n",format_bytes(info.final_size));
	printf("failed allocations: %lu\n",info.failed_allocations);
	printf("last allocation size: %s\n",format_bytes(info.last_allocation_size));
}

static char *format_bytes(long long int bytes){
	static char buffer[1024] = {0};
	const char *suffix_table[] = {
		"KB","MB","GB","TB"
	};
	int i = 0;
	float display_bytes = bytes;
	for (;i < sizeof(suffix_table)/sizeof(char *);){
		if ((display_bytes / 1000) < 1) break;
		display_bytes /= 1000;
		i++;
	}
	snprintf(buffer,sizeof(buffer),"%.1f%s",display_bytes,suffix_table[i]);
	return buffer;
}

int main(){
	void *p = malloc(200);
	free(p);
	void *q = calloc(8,20);
	free(q);
}
