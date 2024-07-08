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
			//printf("getting char\n");
			character = fgetc(doc);
			//printf("got char\n");
			if (feof(doc)){
				done = 1;
				break;
			}
			//printf("got %c\n",character);
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
		//printf("adding null...\n");
		line[line_length] = '\0';
		//printf("realocating line_length\n");	
		lines->line_length = realloc(lines->line_length,sizeof(int)*(line_count+1));
		//printf("updating line_length[%d]\n",line_count);
		lines->line_length[line_count] = line_length;
		line_count++;
		//printf("reallocing lines to %d*sizeof(char*)\n",line_count);
		lines->lines = realloc(lines->lines,sizeof(char*)*line_count);
		//printf("allocating %d chars to index %d of lines\n",line_length+1,line_count-1);
		lines->lines[line_count-1] = malloc(sizeof(char)*(line_length+1));
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
