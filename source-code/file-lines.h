#ifndef lines_h_
#define lines_h_
#include <stdio.h>
struct lines{
	int line_count; //number of lines
	int *line_length; //array of the length of each line
	char **lines; //array of pointers to strings of each line
};
void get_lines(struct lines *lines,FILE* doc);
void free_lines(struct lines *lines);
#endif
