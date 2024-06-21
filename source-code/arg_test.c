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
	return 0;
}
