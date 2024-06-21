#include <stdio.h>
#include <string.h>
int letter_count(int argc,char *argv[]);
int main(int argc,char *argv[]){
	if (argc==1){
		return 1; //not enough arguments provided
	}
	printf("%d\n",letter_count(argc,argv));
}
int letter_count(int argc, char *argv[]){
	int total=argc-2;
	for (int i=1;i<argc;i++){
		total += (int)strlen(argv[i]);
	}
	return total;
}
