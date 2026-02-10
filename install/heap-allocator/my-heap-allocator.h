#ifndef _MHA_H
#define _MHA_H

#include <stddef.h>

void *mha_alloc(size_t size);
void mha_free(void *ptr);

#endif
