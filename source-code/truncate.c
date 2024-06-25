#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char* argv[]){
	if (argc != 3){
		fprintf(stderr,"incorrect number of arguments.\n");//return error in stderr
		return 1;
	}
	printf("arg1:%s\n",argv[1]);
	int *truncate_amount = malloc(sizeof(int));//these are all pointers so i could learn how to use them. sorry it is not very efficient.
	*truncate_amount = atoi(argv[1]);
	int *total_length = malloc(sizeof(int));
	*total_length = strlen(argv[2]);
	int *new_length = malloc(sizeof(int));
	*new_length = *total_length-*truncate_amount;//calculate sizings for new string
	//printf("removing %d chars\n",*truncate_amount);
	//printf("total length:%d\nnew length:%d\n",*total_length,*new_length);

	char *new_string = malloc(((int)sizeof(char)*(*new_length))+1);
	for (int i=0;i<*new_length;i++){
		new_string[i] = argv[2][i];
	}
	new_string[*new_length] = '\0';//terminate new string with null
	printf("%s\n",new_string);

	free(new_string);
	free(total_length);
	free(new_length);
	free(truncate_amount);
	return 0;
}
