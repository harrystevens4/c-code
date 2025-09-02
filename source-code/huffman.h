#ifndef _HUFFMAN_H
#define _HUFFMAN_H

#include <stddef.h>

//sets output to a malloc'd pointer
size_t hfmn_compress(const char data[],size_t len,char **output);
size_t hfmn_decompress(char data[],size_t len,char **output);

#endif
