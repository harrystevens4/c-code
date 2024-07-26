#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <ncurses.h> // use -lncursesw when compiling

//defining globals
int terminal_height;
int terminal_width;
int centre_x;
int centre_y;

void display_popup(const char *text);
void get_term_info();

int main(){
	setlocale(LC_ALL,"");//unicode fonts
	
	//setting up variables
	get_term_info();

	//setting up multi purpose buffers
	char *buffer;
	buffer = malloc(sizeof(char)*(terminal_width+1)); //initialise to lise of screen width

	//init ncurses related things
	initscr();
	cbreak();
	refresh();
	display_popup("This program will now attempt to connect to the mail daemon.");
	sleep(1);
	endwin();

	//cleanup
	free(buffer);
	return 0;
}
void get_term_info(){
	terminal_height = LINES;
	terminal_width = COLS;
	centre_x = terminal_width/2;
	centre_y = terminal_height/2;
}
void display_popup(const char *text){
	//figure out perameters from text
	int height = 4;
	int width = 0;
	int y;
	int x;
	for (int i = 0;i<strlen(text);i++){
		height++;
	}
	x = terminal_width - (width/2);
	y = terminal_height - (height/2);
	//make the window
	WINDOW *popup_window;
	popup_window = newwin(height,width,y,x);
	box(popup_window,0,0);
	mvwprintw(popup_window,0,0,"%sepic and based",text);
	wrefresh(popup_window);
	refresh();
	getch(); //wait for input
	delwin(popup_window);//remove the window
}
