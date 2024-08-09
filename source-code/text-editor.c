#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#define MAX_BUFFER 1024
char filename[MAX_BUFFER];
WINDOW *text;
WINDOW *popup;//for popups like the save dialogue and quit dialogue
char *document; //holds all the chars in the document
int document_length;
int cursor_position; //position in document string
FILE *open_file;

void display_status_bar();
void render_screen();
void render_text();
void render_cursor();
int quit_dialogue();
int load_file(char *filename);

int main(int argc, char** argv){
	//setting globals
	snprintf(filename,MAX_BUFFER,"new");//set default filename for when one is not opened
	int ch;
	document_length = 0;
	document = malloc(sizeof(char));
	document[0] = '\0';

	//load a file if requested
	if (argc > 1){
		printf("Loading file...\n");
		load_file(argv[1]);
	}

	//ncurses setup
	setlocale(LC_ALL,"");//important for displaying the unicode characters
	initscr();


	noecho();


	cbreak();
	raw();//capture things like ctrl-c
	keypad(stdscr,1);
	curs_set(1);//turn cursor to visible
	text = newwin(LINES-2,COLS-2,1,1);//height width y x

	//wrefresh(text);

	render_screen();

	for (;;){
		//mainloop
		//get input
		ch = getch();
		//keybind logic
		if (ch == 3){//ctrl-c
			break;
		}else if(ch == KEY_BACKSPACE){//handling deleting chars
			if (document_length-1 >= 0 && cursor_position > 0){ //check its legal to backspace
				document_length--;
				cursor_position--;
				document[document_length] = '\0';
				document = realloc(document,sizeof(char)*(document_length+1));
			}
		}else if(ch == KEY_LEFT){//navigating with arrow keys
			if (cursor_position > 0){//check we are not at the beginning
				cursor_position--;
			}
		}else if(ch == KEY_RIGHT){
			if (cursor_position < document_length){//check we dont overspill
				cursor_position++;
			}
		}else if(ch == KEY_UP){
			do {
				if (cursor_position < 1){
					break;
				}
				cursor_position--;
			}
			while (document[cursor_position] != '\n');
		}else if(ch == KEY_DOWN){
			do {
				if (cursor_position >= document_length){
					break;
				}
				cursor_position++;
			}
			while (document[cursor_position] != '\n');
		}else{
			document_length++;
			cursor_position++;
			document = realloc(document,sizeof(char)*(document_length+1));
			document[cursor_position-1] = (char)ch;
			document[document_length] = '\0';
		}
		//mvprintw(2,1,"%c:document length %d, reallocating to %d bytes, adding char %c",(char)ch,document_length,(sizeof(char)*(document_length+1)),document[document_length-1]);
		render_text();
	}
	//cleanup
	free(document);
	delwin(text);
	endwin();
}
void display_status_bar(){
	//print logo and any other info
	int buffer_size = COLS-1;//only make buffer as big as it needs to be
	char *buffer = malloc(sizeof(char)*(buffer_size));

	snprintf(buffer,buffer_size,"| Rufus text - [%s] |",filename);

	mvprintw(0,2,buffer);//print the status line
	free(buffer);
}
void render_cursor(){
	int cursor_x = 0;//physical x coord
	int cursor_y = 0;//physical y coord
	for (int i = 0; i < cursor_position; i++){//figure out how many newlines there are
		cursor_x++;
		if (document[i] == '\n' || cursor_x >= COLS-2){//detect when newline and also when text starts to wrap
			cursor_y++;
			cursor_x = 0;
		}
	}
	mvwprintw(text,cursor_y,cursor_x,"\0");// null char does not show up so the cursor is moved here without pringing anything visible to the screen
	wrefresh(text);
}
void render_text(){
	int y = 0;
	int x = 0;
	wclear(text);
	for (int i = 0; i < document_length; i++){
		if (document[i] == '\n'){//carrage return and newline when newline char found
			y++;
			x = 0;
			continue;
		}
		if (x >= COLS-2){//detect if the text would go ofscreen and wrap it round
			y++;
			x = 0;
		}
		mvwprintw(text,y,x,"%c",document[i]);//print each char individualy
		x++;
	}
	render_cursor();
}
void render_screen(){
	//cleanup anything resudual onscreen
	clear();
	wclear(text);
	//redraw everything
	box(stdscr,0,0);
	display_status_bar();
	refresh();//overwrites whole screen when rendering
	render_text();
}
int quit_dialogue(){//return 1 for quit and 0 for cancel
}
int load_file(char *filename){
	char char_buffer;

	//check file can be opened and open it
	open_file = fopen(filename,"r");
	if (open_file == NULL){
		return 1;
	}

	//remove existing content of document ready for new content
	free(document);
	document = malloc(sizeof(char));
	document_length = 0;

	//read the file
	for (;;){
		char_buffer = fgetc(open_file);
		document = realloc(document,sizeof(char)*(document_length+1));
		if (feof(open_file)){//leave loop when end of file reached
			break;
		}
		document[document_length] = char_buffer;
		document_length++;
	}
	document[document_length] = '\0';
}
