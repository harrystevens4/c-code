#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ssize_t base32_decode(char *input, size_t input_len, char *output){
	//base32 char mapping macro
	#define B32D(c) (\
		(c >= 'A' && c <= 'Z') ? (c - 65) : (\
			(c >= '2' && c <= '7') ? (c - 50 + 26) : (0)\
		)\
	)
	//valid length of input
	if (input_len % 8 != 0) return -1;
	//00000000 00000000 00000000 00000000 00000000
	//|---||----||---||----||----||---||----||---|   
	size_t output_len = 0;
	//work with 5 bytes at a time
	for (size_t i = 0; i < input_len;){
		char i0 = B32D(input[i+0]),
			i1 = B32D(input[i+1]),
			i2 = B32D(input[i+2]),
			i3 = B32D(input[i+3]),
			i4 = B32D(input[i+4]),
			i5 = B32D(input[i+5]),
			i6 = B32D(input[i+6]),
			i7 = B32D(input[i+7]);
		output[output_len+0] = ((i0 << 3) | (i1 >> 2)            ) & 0xff;
		output[output_len+1] = ((i1 << 6) | (i2 << 1) | (i3 >> 4)) & 0xff;
		output[output_len+2] = ((i3 << 4) | (i4 >> 1)            ) & 0xff;
		output[output_len+3] = ((i4 << 7) | (i5 << 2) | (i6 >> 3)) & 0xff;
		output[output_len+4] = ((i6 << 5) | (i7     )            ) & 0xff;
		i += 8;
		output_len += 5;
	}
	//calculate the length
	long input_no_pad_len = (long)(strchrnul(input,'=') - input);
	return (input_no_pad_len*5)/8;
}

ssize_t base64_decode(char *input, size_t input_len, char *output){
	//base64 char mapping macro
	#define B64D(c) (\
		/*if*/ (c >= 'A' && c <= 'Z') ? /*return*/(c - 65) : (\
		/*else if*/ (c >= 'a' && c <= 'z') ? /*return*/(c - 97 + 26) : (\
		/*else if*/ (c == '+') ? /*return*/(62) : (\
		/*else if*/	(c == '/') ? /*return*/(63) : (\
		/*else if*/	(c >= '0' && c <= '9') ? /*return*/(c - 48 + 26 + 26) : \
		/*else*/ /*return*/(0)\
		))))\
	)
	//valid length of input
	if (input_len % 4 != 0) return -1;
	//00000000 00000000 00000000
	//|----||-----||-----||----|
	size_t output_len = 0;
	//work with 3 bytes at a time
	for (size_t i = 0; i < input_len;){
		output[output_len+0] = ((B64D(input[i+0]) << 2) | (B64D(input[i+1]) >> 4)) & 0xff;
		output[output_len+1] = ((B64D(input[i+1]) << 4) | (B64D(input[i+2]) >> 2)) & 0xff;
		output[output_len+2] = ((B64D(input[i+2]) << 6) | (B64D(input[i+3])     )) & 0xff;
		i += 4;
		output_len += 3;
	}
	//calculate the length
	long input_no_pad_len = (long)(strchrnul(input,'=') - input);
	return (input_no_pad_len*6)/8;
}
