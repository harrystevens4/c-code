#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "args.h" //compile with args.c
int letter_count(struct args arguments);
int main(int argc,char *argv[]){
	if (argc==1){
		printf("0\n");//0 letters / words
		return 1; //not enough arguments provided
	}
	struct args arguments; //holds all the args info
	parse_args(argc,argv,&arguments);
	int i = 0;
	for (i=0;i<arguments.number_single;i++){
		if (arguments.single[i] == 'w'){
			;
		}else{
			;
		}
	}
	if (i==0){
		printf("%d\n",letter_count(arguments));
	}

	free(arguments.other); //freeing from the arg parser
}

int letter_count(struct args arguments){
	int total = arguments.number_other-1;
	for (int i=0;i<arguments.number_other;i++){
		total += (int)strlen(arguments.other[i]);
	}
	return total;
}
