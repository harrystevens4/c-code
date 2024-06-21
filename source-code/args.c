#include "args.h"
#include <stdio.h>
#include <string.h>
void parse_args(int argc, char *argv[], struct args *arguments){
	int multi_count = 0; //how many single character argumens there are
	int single_count = 0; //how many multicharacter arguments
	int other_count = 0; // for non hyphen arguments
	for (int i=1; i<argc;i++){
		if (argv[i][0]=='-'){//check for args starting with -
			if (argv[i][1]=='-'){//check for args that start with --
				strcpy(arguments->multi[multi_count],argv[i]);
				multi_count++;
			}else{ //triggers for single args
				for (int character = 1;character<strlen(argv[i]);character++){
					arguments->single[single_count] = argv[i][character];
					single_count++;
				}
			}
		}else{//non dashed args
			strcpy(arguments->other[other_count],argv[i]);
			other_count++;
		}
	}
	arguments->number_single = single_count;
	arguments->number_multi = multi_count;
	arguments->number_other = other_count;
}
