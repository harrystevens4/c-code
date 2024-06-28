#include <stdio.h>
#include <stdlib.h>
#include "file-lines.h"
#include <string.h>
void get_lines(struct lines *lines,FILE* doc){
	/* initialising variables */
	char *line;
	char character;
	int done = 0; //if the file has finnished reading
	int line_length = 0;
	int line_count = 0;
	lines->lines = malloc(sizeof(char*));
	line = malloc(sizeof(char)*(1));
	lines->line_length = malloc(sizeof(int)); //set up array of line lengths
	for (;;){ //itterate trhough the whole document
		for (;;){ //itterate through one line
			character = fgetc(doc);
			if (feof(doc)){
				done = 1;
				break;
			}
			line_length++;
			line = realloc(line,sizeof(char)*(line_length+1));
			line[line_length-1] = character;
			if (character == '\n'){ //finnish at newline char
				break;
			}
		}
		if (done){
			break;
		}
		line[line_length] = '\0';	
		lines->line_length = realloc(lines->line_length,sizeof(int)*(line_count+1));
		lines->line_length[line_count] = line_length;
		line_count++;
		lines->lines = realloc(lines->lines,sizeof(char*)*line_count);
		lines->lines[line_count-1] = realloc(lines->lines[line_count-1],sizeof(char)*(line_length+1));
		strcpy(lines->lines[line_count-1],line);
		line_length = 0;

	}
	lines->line_count = line_count;
	/* cleanup */
	free(line);
}
void free_lines(struct lines *lines){
	free(lines->line_length);
	for (int i=0;i<lines->line_count;i++){
		free(lines->lines[i]);
	}
	free(lines->lines);
}
