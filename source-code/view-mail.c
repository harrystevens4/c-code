#include <stdlib.h>
#include "daemon-toolkit.h" //compile with daemon-toolkit.c
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include <ncurses.h> // use -lncursesw when compiling
#include <curses.h> //for macro definitions

//defining globals
volatile int terminal_height;
volatile int terminal_width;
int centre_x;
int centre_y;

int display_popup(const char *text,const char *tooltip);
void get_term_info();

int main(){
	non_lethal_errors = 1;
	setlocale(LC_ALL,"");//unicode fonts
	
	//setting up variables

	//setting up multi purpose buffers
	char *buffer;
	buffer = malloc(sizeof(char)*(terminal_width+1)); //initialise to lise of screen width

	//init ncurses related things
	initscr();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(0);
	get_term_info();
	cbreak();
	refresh();
	clear();
	refresh();
	
	//connect to the daemon
	int key;
	int status;
	int index = 0;
	int socket = connect_named_socket("/tmp/mail-manager.socket");
	if (socket<0){
		display_popup("Could not connect to mail-manager daemon.","<Press any key to exit>");
		endwin();
		return 1;
	}
	int delete_socket;
	char *header;
	char *body;
	char *full_mail;
	send_string(socket,"view");
	int mail_count = receive_int(socket);
	if (mail_count == 0){
		send_string(socket,"done");
		display_popup("There is no new mail to view.","<Press any key to exit>");
		clear();
		refresh();
		endwin();
		return 0;
	}
	for ( ; ; ){
		//mainloop
		send_string(socket,"more");
		send_int(socket,index);//ask for mail that the user wanted

		//get header and body
		receive_string(socket,&header);
		receive_string(socket,&body);

		//create popup
		full_mail = malloc((strlen(body)+strlen(header)+2)*sizeof(char));
		sprintf(full_mail,"%s\n-----\n%s",header,body);
		clear();
		refresh();
		mvprintw(0,(int)((terminal_width/2)-4),"⇐ %d/%d ⇒",index+1,mail_count);
		key = display_popup((const char *)full_mail,"<next>");

		//cleanup before next transmition
		free(header);
		free(body);
		free(full_mail);
		if (key == 'q'){
			break;
		}else if (key == KEY_LEFT){
			if (index-1>=0){
				index--;
			}
		}else if (key == KEY_RIGHT){
			if (index+1<mail_count){
				index++;
			}
		}else if (key == 'd'){
			clear();
			refresh();
			if (display_popup("Are you sure you want to delete this?","<y/n>") == 'y'){
				delete_socket = connect_named_socket("/tmp/mail-manager.socket");
				//delete mail
				send_string(delete_socket,"delete");
				send_int(delete_socket,index);
				status = receive_int(delete_socket);
				close(delete_socket);
				mail_count--;
				if (index>1){
					index--;
				}
			}
		}
		if (mail_count<1){
			clear();
			refresh();
			display_popup("No more mail to display.","<press any key to exit>");
			break;
		}

	}
	send_string(socket,"done");

	//cleanup
	close(socket);
	free(buffer);
	endwin();
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
int display_popup(const char *text,const char *tooltip){
	//figure out perameters from text
	unsigned int height = 5;
	unsigned int width = 4;
	int y = 0;
	int x = 0;
	int length = 0; 
	int max_length = strlen(tooltip);
	for (int i = 0;i<strlen(text);i++){
		length++;
		if ((text[i] == '\n') && (text[i+1] != '\0')){
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
	//make sure there is at least a 1 char margin on each side
	if (x<1){
		x=1;
	}
	if (y<1){
		y=1;
	}
	if (width>terminal_width-2)
		width = terminal_width - 2;
	if (width>terminal_width-2)
		width = terminal_height - 2;
	//make the window
	unsigned int text_x = x+2;
	unsigned int text_y = y+1;
	WINDOW *popup_window;
	popup_window = newwin(height,width,y,x);
	box(popup_window,0,0);
	wrefresh(popup_window);
	for (int i = 0; i<strlen(text); i++){
		if (text[i] == '\n'){
			text_y++;
			text_x = x+2;
			continue;
		}
		mvprintw(text_y,text_x,"%c",text[i]);
		text_x++;
	}
	mvhline(y+height-3,x+2,0,width-4);
	mvprintw(y+height-2,x+2,"%s",tooltip);
	refresh();
	//refresh();
	int key = getch(); //wait for input
	delwin(popup_window);//remove the window
	return key;
}
