#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int str_len(char string[]){
	int count = 0;
	while (1){
		if (string[count] == '\0'){
			return count;
		}else{
			count++;
		}
	}
}
int *getstringpos(char string[],char substring[]) {
	int i;	
	int j;
	int substring_length;
	int string_length;
	int start;
	int end;
	substring_length=str_len(substring);
	string_length=str_len(string);
	//printf("strlen:%d,substrlen:%d\n",substring_length,string_length);
	for (i=0;i<string_length;i++){
		for (j=0;j<substring_length;j++){
			//printf("comparing %c to %c\n",string[i],substring[j]);
			if (string[i]==substring[j]){
				end=i;
				start=i-substring_length+1;
				//printf("letter match!\n");
			}else{
				break;
			}
		i++;
		}
	}
	//printf("start:%d,end:%d\n",start,end);
	int *result = malloc(1024);
	result[0]=start;
	result[1]=end;
	return result;
}

void main(int argc, char* argv[]){
	int i;
	if (argc != 3){
		printf("expected 2 arguments, but got %d",argc-1);
	}else{
		char string[100];
		strcpy(string,argv[1]);
		char substring[100];
		strcpy(substring,argv[2]);
		printf("string:%s\n",string);
		printf("substring:%s\n",substring);
		int *result;
		result = getstringpos(string,substring);
		printf("start:%d,end:%d\n",result[0],result[1]);
		free(result);
	}
}

