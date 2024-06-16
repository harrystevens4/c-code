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

char *remove_substring(char string[],int start,int end){
	int length = str_len(string)-(+end-start+1);
	//printf("new string length:%d\n",length);
	char result[length+1];
	result[length] = '\0';
	int count = 0;
	for (int i=0;i<str_len(string);i++){
		//printf("%c\n",string[i]);
		if ( ! (i >= start && i<= end)) {
			result[count] = string[i];
			count++;
		}

	}
	//printf("result:%s\n",result);
	char *end_result = malloc(length+1); //+1 for the '\0' at the end
	strcpy(end_result,result);
	return end_result;
}

void main(int argc, char* argv[]){
	int i;
	if (argc != 3){
		fprintf(stderr,"expected 2 arguments, but got %d\n",argc-1);
	}else{
		char string[100];
		strcpy(string,argv[1]);
		char substring[100];
		strcpy(substring,argv[2]);
		//printf("string:%s\n",string);
		//printf("substring:%s\n",substring);
		int *result;
		result = getstringpos(string,substring);
		//printf("start:%d,end:%d\n",result[0],result[1]);
		char *new_string;
		new_string = remove_substring(string,result[0],result[1]);
		printf("%s\n",new_string);
		free(result);
		free(new_string);
	}
}

