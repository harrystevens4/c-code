#include <stdio.h>
#include <stdlib.h>
#include "file-lines.h" //compile with file-lines.c
#include "args.h" //compile with args.c
int main(int argc, char* argv[]){
	struct args arguments;
	struct lines lines;
	parse_args(argc,argv,&arguments);
	/* checks */
	if (arguments.number_other != 1){
		fprintf(stderr,"incorrect number of arguments. expected 1 arg of <filename>\n");
		return 1;
	}
	FILE* doc;
	doc = fopen(arguments.other[0],"r");
	if (doc == NULL){
		fprintf(stderr,"could not open file\n");
		return 2;
	}
	/* process file */
	//printf("getting lines...\n");
	get_lines(&lines,doc);
	//printf("got lines...\n");
	//for (int i=0;i<lines.line_count;i++){
	//	printf("line %d has a char count of %d and content of:%s",i,lines.line_length[i],lines.lines[i]);
	//}
	for (int i=0;i<arguments.number_single;i++){
		switch (arguments.single[i]){
			case 'c': //remove "c"oments from output
				/* itterate through each line saving each char and stopping once a # is found. */
				for (int line=0;line<lines.line_count;line++){
					for (int letter=0;letter<lines.line_length[line];letter++){
						if (lines.lines[line][letter]=='#'){
							if (letter == 0){//full line comments
								lines.lines[line][letter] = '\0'; //slap a null byte to terminate the string in place of the '#'
							}else{//partial line comments
								lines.lines[line][letter] = '\n';
								lines.lines[line][letter+1] = '\0'; //replace newline we cut off
							}
						}
					}
				}
			case 'n'://remove all newlines
				for (int line=0;line<lines.line_count;line++){
					if (lines.lines[line][lines.line_length[line]-1]=='\n'){
						lines.lines[line][lines.line_length[line]-1] = '\0';//set newline to null to terminate the string
					}
				}

		}
	}
	/* output */
	for (int i=0;i<lines.line_count;i++){
		printf("%s",lines.lines[i]);
	}
	/* cleanup */
	free_args(&arguments);
	free_lines(&lines);
	fclose(doc);
	return 0;
}
