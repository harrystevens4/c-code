
# How to use file-lines.c


## Initialising it

1. first off, make sure to include "file-lines.h". This includes the definition of the lines struct.
2. the next thing to do is initialise the struct that will hold all the data about the file. this can be done with `struct lines <name>`. an example would be `struct lines lines` or `struct lines file-lines`.
3. open a file in read mode
4. run `get_lines(&<lines struct name>,<file pointer>)`

## using it

Once it has been initialised, the struct contains all the variables relating to the document. this includes:
1. `int line_count` number of lines in the document
2. `int *line_length` a pointer to an array of integers representing the line lengths of each line
3. `char **lines` an array of pointers to strings of each seperate line. (can be addressed such as `lines.lines[0]` to acces line 1)

## cleaning up

simply run the `free_lines(&<lines struct name>)` to free up the variables and memory related to it.

## examples

	#include "file-lines.h"
	#include <stdio.h>
	int main(){
		struct lines lines;
		FILE* doc;
		doc = fopen("hello.txt","r");
		get_lines(&lines,doc);
		for (int i=0;i<lines.line_count;i++){
			printf("line %d has %d characters(including newline):%s",i,lines.line_length[i],lines.lines[i]);
		}
	}
