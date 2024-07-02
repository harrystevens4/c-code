#include <ncurses.h>
#include <curses.h> //for the ACS_VLINE defs and such
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
int main(){
	/* initialisation */
	setlocale(LC_ALL,""); //for unicode chars
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	const int term_width = COLS;
	const int term_height = LINES;
	const int max_height = 15;
	const int min_height = 10;
	const int max_width = 40;
	const int min_width = 15;
	int width;
	int height;
	/* height setup */
	if (term_height<=max_height){
		if(term_height>=min_height){
			height = term_height;
		}else{
			height = min_height;
		}
	}else{
		height = max_height;
	}
	/* width logistics */
	if (term_width<=max_width){
		if(term_width>=min_width){
			width = term_width;
		}else{
			width = min_width;
		}
	}else{
		width = max_width;
	}
	int x=(int)((term_width/2)-(width/2));
	int y=(int)((term_height/2)-(height/2));
	int mid_x = x+(int)(width/2);
	int input;
	int exit = 0;
	int selected = 0;
	refresh();
	WINDOW *win;
	win = newwin(height,width,y,x);
	box(win,0,0);
	mvvline(y+1,mid_x,ACS_VLINE,height-2);//centre partition
	wrefresh(win);
	while (!exit){
		/* selecting options */

		//box(win,0,0);
		wrefresh(win);
		refresh();
		/* input processing */
		input = getch();
		switch (input){
			case KEY_F(1):
				exit=1;
				break;
		}
	}
	endwin();
	printf("finnished successfully\n");
	printf("term height %d term width %d height %d width %d\n",term_height,term_width,height,width);
	return 0;
}
