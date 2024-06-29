#include "daemon-core.h"
#include <stdlib.h>

void free_buffer(struct buffer *buffer){
	free(buffer->lengths);
	for (int i=0;i<buffer->buffer_length;i++){
		free(buffer->buffer[i]);
	}
	free(buffer->buffer);
}

void free_locks(struct locks *locks){
	free(locks->lock);
}
