#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define INITIAL_HEAP_SIZE 1024

struct chunk_header {
	void *next;
	void *prev;
	uint64_t checksum;
	uint64_t chunk_size; //usable data (not including the header)
	uint8_t is_free; //0 = allocated 1 = free
};

static int initialised = 0;

struct {
	void *heap_start;
	size_t heap_size;
} heap_info;

uint64_t calculate_chunk_checksum(void *chunk_start){ //where the header is put
	return (uint64_t)chunk_start * 52 - 19;
}

int split_chunk(void *chunk_start, size_t first_half_size){
	//is it large enough
	struct chunk_header *header = chunk_start;
	//acount for the new chunk header we will have to create
	if (header->chunk_size < (first_half_size + sizeof(struct chunk_header))){
		return -1;
	}
	//create the new chunk
	size_t old_chunk_size = header->chunk_size;
	struct chunk_header *new_chunk_header = chunk_start + sizeof(struct chunk_header) + first_half_size;
	memset(new_chunk_header,0,sizeof(struct chunk_header));
	new_chunk_header->chunk_size = old_chunk_size - sizeof(struct chunk_header);
	new_chunk_header->prev = chunk_start;
	new_chunk_header->next = header->next;
	new_chunk_header->is_free = header->is_free;
	//update the old chunk
	header->chunk_size = first_half_size;
	header->next = new_chunk_header;
	return 0;
}

void init(){
	//====== mmap a small heap to start with ======
	memset(&heap_info,0,sizeof(heap_info));
	void *heap_start = mmap(NULL,INITIAL_HEAP_SIZE,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
	heap_info.heap_start = heap_start;
	heap_info.heap_size = INITIAL_HEAP_SIZE;
	if (heap_start == NULL){
		perror("mmap");
		return;
	}
	//====== create an initial free block ======
	struct chunk_header *first_chunk_header = heap_start;
	memset(first_chunk_header,0,sizeof(*first_chunk_header));
	first_chunk_header->chunk_size = heap_info.heap_size-sizeof(struct chunk_header);
	first_chunk_header->is_free = 1;
	first_chunk_header->next = NULL;
	first_chunk_header->prev = NULL;
	first_chunk_header->checksum = calculate_chunk_checksum(first_chunk_header);
	//====== cleanup ======
	printf("heap initialised\n");
	initialised = 1;
}

void *mha_alloc(size_t size){
	if (!initialised) init();
	if (!initialised) return NULL; //initialisation failed
	//traverse chunks
	for (struct chunk_header *chunk = heap_info.heap_start; chunk != NULL; chunk = chunk->next){
		//chunk is free?
		if (chunk->is_free == 1){
			//attempt to partition it
			int result = split_chunk(chunk,size);
			//not enough space
			if (result < 0) continue;
			else return chunk;
		}
	}
	return NULL;
}
void mha_free(void *ptr){
	if (!initialised) init();
	if (!initialised) return; //initialisation failed
}
