#include <stdio.h>
#include <alloca.h>
#include <stdlib.h>
#include <string.h>

void print_help();
ssize_t base32_decode(char *input, size_t input_len, char *output);

int main(int argc, char **argv){
	//====== handle arguments ======
	if (argc < 2){
		fprintf(stderr,"Please provide a command. See \"%s help\" for help\n",argv[0]);
		return EXIT_FAILURE;
	}
	if (strcmp(argv[1],"help") == 0){
		print_help();
		return EXIT_SUCCESS;
	}
	if (argc < 3){
		fprintf(stderr,"Please provide data. See \"%s help\" for help\n",argv[0]);
		return EXIT_FAILURE;
	}
	//====== decode ======
	char *input = argv[2];
	size_t input_len = strlen(argv[2]);
	//validate
	if (input_len % 8 != 0){
		fprintf(stderr,"Invalid input\n");
		return EXIT_FAILURE;
	}
	//im using alloca because it seems cool
	ssize_t output_len = (input_len*5) / 8;
	char *output = alloca(output_len);
	memset(output,0,output_len);
	output_len = base32_decode(input,input_len,output);
	if (output_len < 0){
		fprintf(stderr,"Could not decode\n");
		return EXIT_FAILURE;
	}
	printf("%.*s\n",(int)output_len,output);
	return EXIT_SUCCESS;
}

void print_help(){
	printf("usage: base-n <command> <data>\n");
	printf("command is either encode or decode\n");
}

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
