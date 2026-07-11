#ifndef _BASE_N_H
#define _BASE_N_H

#include <stddef.h>

ssize_t base32_decode(char *input, size_t input_len, char *output);
ssize_t base64_decode(char *input, size_t input_len, char *output);

#endif
