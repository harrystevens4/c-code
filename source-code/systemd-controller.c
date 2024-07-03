#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <curses.h> //for the ACS_VLINE defs and such
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
int main(){
	/* initialisation */
	setlocale(LC_ALL,""); //for unicode chars
	initscr();
	start_color();
	init_pair(1,COLOR_WHITE,COLOR_BLACK);//non highlighted
	init_pair(2,COLOR_WHITE,COLOR_RED);//highlighted
	cbreak();
	curs_set(0);//no cursor
	keypad(stdscr, TRUE);
	noecho();
	
	/* file detection */
	char **files = malloc(sizeof(char*));
	int count = 0;
	DIR *cur_dir;
	struct dirent *dir;
	cur_dir = opendir("/lib/systemd/system");
	if (cur_dir) {
		while ((dir = readdir(cur_dir)) != NULL) {
			if ((dir->d_name != "..") && (dir->d_name != ".")){
				count++;
				files = realloc(files,sizeof(char*)*count);
				files[count-1] = malloc(sizeof(char)*(strlen(dir->d_name)+1));
				strcpy(files[count-1],dir->d_name);
				printf("%s\n", dir->d_name);
			}
		}
		closedir(cur_dir);
	}

	const int term_width = COLS;
	const int term_height = LINES;
	const int max_height = 15;
	const int min_height = 10;
	const int max_width = 50;
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
	int input;
	int exit = 0;
	int selected = 0; //selection menu (left side)
	int action = 0; //action menu (right side)
	int cur_selected = 0;
	int offset = 0;
	const int number_of_actions = 4;
	const int action_width = 13; //width of the right side actions
	const int selection_width = width - action_width - 3;
	const char actions[4][15] = {"<start>","<stop>","<enable>","<disable>"};//right side action menu options
	int character=0;
	char letter;
	int ellipses = 0;
	int mid_x = x+width-action_width; //x of mid partition
	refresh();
	WINDOW *win;
	win = newwin(height,width,y,x);
	box(win,0,0);
	mvvline(y+1,mid_x,ACS_VLINE,height-2);//centre partition
	wrefresh(win);
	while (!exit){
		/* selecting options */
		for (int i=0;i<number_of_actions;i++){
			if (i==action){
				attrset(COLOR_PAIR(2));
			}
			mvprintw(i+y+1,mid_x+2,"%s",actions[i]);
			attrset(COLOR_PAIR(1));
		}

		for (int i=0;i<height-2;i++){
			if (i==cur_selected){
				attrset(COLOR_PAIR(2));
			}
			for (character=0;character<selection_width;character++){
				letter = files[i+offset][character];
				if (letter=='\0'){
					ellipses = 0;
					break;
				}else{
					ellipses = 1;
				}
				mvprintw(i+y+1,x+2+character,"%c",letter);
			}
			for (;character<selection_width;character++){
				mvprintw(i+y+1,x+2+character," ");
			}
			if (ellipses){
				mvprintw(i+y+1,x+selection_width-1,"...");
			}
			attrset(COLOR_PAIR(1));
		}
		/* refresh */
		refresh();
		/* input processing */
		input = getch();
		switch (input){
			case KEY_F(1):
				exit=1;
				break;
			case 9: //tab
				action++;
				if (action>number_of_actions-1){
					action=0;
				}
				break;
			case 10:
				exit=1;
				break;
			case KEY_UP:
				selected--;
				if (cur_selected-1<0){
					if (offset>0){
						offset--;
					}
				}else{
					cur_selected--;
				}
				break;
			case KEY_DOWN:
				if (selected+1<count){
					selected++;
					if (cur_selected+1>height-3){
						offset++;
					}else{
						cur_selected++;
					}
					break;
				}
		}
	}
	endwin();
	for (int i=0;i<count;i++){
		free(files[i]);
	}
	free(files);

	printf("finnished successfully\n");
	printf("term height %d term width %d height %d width %d\n",term_height,term_width,height,width);
	printf("%d\n",input);
	return 0;
}
