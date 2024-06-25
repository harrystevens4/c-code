#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "args.h"

char* repeat_char(char character,int number);

int main(int argc, char *argv[]){
	struct args arguments;
	parse_args(argc,argv,&arguments);
	struct winsize terminal;
	ioctl(1,TIOCGWINSZ, &terminal);
	int width = terminal.ws_col;
	int height = terminal.ws_row;
	int selected = 0;
	char *empty_row;
	char *lpad;
	char *rpad;
	char *prompt;
	char *base;// these characters that make up the top and bottom: ══════════
	int tpad_num;
	int bpad_num;
	int lpad_num;
	int rpad_num;
	int option_width //the actual width of the options being displayed within the window
	int win_height;
	int win_width=12;//minimum width of ║ :::  ::: ║
	for (int i=0;i<arguments.number_other;i++){
		if (strlen(arguments.other[i])+12 > win_width){
			win_width = strlen(arguments.other[i])+12; //check the width with all the options to find the maximum possible width
		}
	}
	lpad_num = (int)(width/2)-(int)(win_width/2);//half the total width - half the window width
	rpad_num = width-(lpad_num+win_width);//the rest of the screen not takenby the window or lpad
	lpad = malloc((sizeof(char)*rpad_num)+1);// null for +1
	for (int i=0;i<lpad_num;i++){
		lpad[i] = ' ';
	}
	lpad[lpad_num] = '\0';

	rpad = malloc((sizeof(char)*rpad_num)+1);
	for (int i=0;i<rpad_num;i++){
		rpad[i] = ' ';
	}
	rpad[rpad_num] = '\0';

	empty_row = malloc((sizeof(char)*width)+1);// +1 for the null byte at the end
	for (int i = 0;i<width;i++){
		empty_row[i] = ' ';
	}
	empty_row[width] = '\0';
	
	prompt_width = (win_width-12);
	prompt = malloc(sizeof(char)*(prompt_width+1));
	strcpy(prompt,arguments.other[0]); //first other arg is the prompt
	for (int i=0;i<prompt_width){
	




		//continue here





	}

	//base = malloc((sizeof(char)*(win_width-2))+1);
//	for (int i = 0;i<win_width-2;i++){
//		base[i] = '\x2550'; // ═
//	}
	//base[win_width-2] = '\0';


	//for (int i=0;i<arguments.number_single;i++){
	//	if (arguments.single[i]=='c'){ //c for confirm (yes or no)
	//		printf("yes/no mode selected\n");
	//	}else{
	//		;
	//	}
	//}
	win_height = arguments.number_other+4;
	tpad_num = (height/2)-(int)(win_height/2);// measure half then remove half the height of the window to indent it
	bpad_num = height-(tpad_num+win_height);// subtract the remaining space to get the bpad
	printf("win_height:%d\ntpad_num:%d\nbpad_num:%d\nwidth:%d\n",win_height,tpad_num,bpad_num,win_width);

	while(1){
		printf("\033[104;97m");
		for (int i=0;i<tpad_num;i++){//print the top padding
			printf("%s",empty_row);
		}
		printf("%s╔",lpad);//l pad and corner
		for (int i=0;i<win_width-2;i++){
			printf("═");//top line mid section
		}
		printf("╗%s\n",rpad);//right corner and r pad
		printf("%s║ ::: %s ::: ║%s\n",lpad,prompt,rpad); //second line
		break;
	}

	free(empty_row);
	//free(base);
	free(lpad);
	free(prompt);
	//free(rpad);
	free_args(&arguments);// frees the massive 2d array holding the other arguments
	return selected; //return the index of the selected option
}
