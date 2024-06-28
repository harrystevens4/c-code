# How to use `args.c`

## Info 

The struct `args` contais 6 variables.
- `int number_single` : number of short (single letter, e.g. -s ) arguments provided
- `int number_multi` : number of long arguments (start with double dash, e.g. --flag)
- `int number_other` : number of arguments that do not start with a dash.
- `char single` : a character string (does not end in null byte) with all the single letter arguments
- `char multi[10][20]` : 2d array of strings of the long arguments. (does not remove the preceding -- at the start of each arg)
- `char **other` : array of pointers to strings of the non dashed arguments

## Initialisation

1. Make sure you have the line `#include "args.h"` at the top of your main file
2. Make sure your `main()` function takes in `main(int argc, char *argv[])`
3. Now you can initialise the struct that will store the formatted arguments. Do this by using `struct args <name>`. For example, `struct args arguments`
4. You can now pass this struct as well as `argc` and `argv` directly into the `parse_args()` function. Continuing from the previous example, `parse_args(argc,argv,&arguments)`

## Making use of the arguments

Now you have initialised it, you can acces the info from within the `args` struct.

### Example on how to use this effectively

	#include "args.h"
	#include <stdio.h>
	int main(int argc, char *argv[]){
		if (argc == 1){
			printf("no arguments given\n");
			return 1; //no arguments passed
		}
		struct args arguments;
		parse_args(argc,argv,&arguments);
		printf("number of long args:%d\nnumber of single args:%d\n",arguments.number_multi,arguments.number_single);
		for (int i = 0;i<arguments.number_single;i++){
			printf("single:%c\n",arguments.single[i]);
		}
		for (int i = 0;i<arguments.number_multi;i++){
                	printf("long:%s\n",arguments.multi[i]);
        	}
       		for (int i=0;i<arguments.number_other;i++){
                	printf("other:%s\n",arguments.other[i]);
        	}
        	return 0;
	}

## Cleaning up

after you are done witht the arguments, run the `free_args(&<arg struct name>)`
