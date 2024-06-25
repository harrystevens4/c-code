#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "args.h" //compile with args.c
int letter_count(struct args arguments);
int word_count(struct args arguments);
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
			printf("%d\n",word_count(arguments));
		}else{
			;
		}
	}
	if (i==0){
		printf("%d\n",letter_count(arguments));
	}

	for (i=0;i<arguments.number_other;i++){
		free(arguments.other[i]);
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
int word_count(struct args arguments){
	int total = 0;
	if (arguments.number_other != 0){
		total = 1;
	}else{
		return 0;
	}
	for (int i = 0;i<strlen(arguments.other[0]);i++){// for each letter in the fisrt string argument
		if (arguments.other[0][i] == ' '){ //we check for the number of spaces to count words
			total++;
		}
	}
	return total;
}
