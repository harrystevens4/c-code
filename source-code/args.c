#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
void parse_args(int argc, char *argv[], struct args *arguments){
	int multi_count = 0; //how many single character argumens there are
	int single_count = 0; //how many multicharacter arguments
	int other_count = 0; // for non hyphen arguments
	//printf("creating other...\n");
	char **other;
	//printf("alocating memory to other...\n");
	other = (char**)malloc(sizeof(char*)*10); // pointer to array of pointers to strings
	//printf("starting size of other %lu\n",sizeof(other));
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
			other_count++;
			other = (char**)realloc(other,other_count*sizeof(char*));
			//printf("realocated other to %d\n",(int)(other_count*sizeof(char*)));
			other[other_count-1] = (char*)malloc((strlen(argv[i])+1)*sizeof(char));
			strcpy(other[other_count-1],argv[i]);
			//printf("size of new string:%ld\n",((strlen(other[other_count-1])+1))*sizeof(char));
			//printf("string stored:%s\n",other[other_count-1]);
		}
	}
	arguments->other = other;
	arguments->number_single = single_count;
	arguments->number_multi = multi_count;
	arguments->number_other = other_count;
}
