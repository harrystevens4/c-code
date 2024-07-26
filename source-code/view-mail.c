#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <ncurses.h> // use -lncursesw when compiling

//defining globals
volatile int terminal_height;
volatile int terminal_width;
int centre_x;
int centre_y;

void display_popup(const char *text);
void get_term_info();

int main(){
	setlocale(LC_ALL,"");//unicode fonts
	
	//setting up variables

	//setting up multi purpose buffers
	char *buffer;
	buffer = malloc(sizeof(char)*(terminal_width+1)); //initialise to lise of screen width

	//init ncurses related things
	initscr();
	get_term_info();
	cbreak();
	refresh();
	display_popup("This program will now attempt to connect to the mail daemon.");
	endwin();

	//cleanup
	free(buffer);
	return 0;
}
void get_term_info(){
	terminal_height = LINES;
	printf("initialised terminal height to %d\n",terminal_height);
	terminal_width = COLS;
	printf("initialised terminal width to %d\n",terminal_width);
	centre_x = terminal_width/2;
	centre_y = terminal_height/2;
}
void display_popup(const char *text){
	//figure out perameters from text
	unsigned int height = 4;
	unsigned int width = 4;
	int y = 0;
	int x = 0;
	int length = 0; 
	int max_length = 0;
	for (int i = 0;i<strlen(text);i++){
		length++;
		if (text[i] == '\n'){
			height++;
			length = 0;
		}
		if (length > max_length){
			max_length = length;
		}
		
	}
	width += max_length;
	x = (centre_x - (width / 2));
	y = (centre_y - (height / 2));
	if (x<0){
		x=0;
	}
	if (y<0){
		y=0;
	}
	//make the window
	WINDOW *popup_window;
	popup_window = newwin(height,width,y,x);
	box(popup_window,0,0);
	wrefresh(popup_window);
	mvprintw(y+1,x+2,"%s",text);
	refresh();
	//refresh();
	getch(); //wait for input
	delwin(popup_window);//remove the window
}
