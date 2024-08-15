#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <string.h>
#define MAX_BUFFER 1024
char filename[MAX_BUFFER];
WINDOW *text;
char *document; //holds all the chars in the document
int document_length;
int cursor_position; //position in document string
int line_offset;//how many lines are not shown above the visible text
FILE *open_file;
int cursor_x = 0;//physical x coord
int cursor_y = 0;//physical y coord
int unsaved = 0;

void display_status_bar();
void render_screen();
void render_text();
int render_cursor();
int quit_dialogue();
int save_dialogue();
int load_file(char *filename);
int save_file();

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
			if (quit_dialogue()){
				break;
			}
		}else if (ch == 19){
			save_dialogue();
			render_screen();
		}else if(ch == KEY_BACKSPACE){//handling deleting chars
			if (document_length-1 >= 0 && cursor_position > 0){ //check its legal to backspace
				for (int i = cursor_position-1; i < document_length; i++){//shuffling characters backwards in the array
					document[i] = document[i+1];
				}
				document_length--;
				cursor_position--;
				document = realloc(document,sizeof(char)*(document_length+1));
				unsaved = 1;
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
			//shuffle array elements to make room
			document_length++;
			document = realloc(document,sizeof(char)*(document_length+1));
			for (int i = document_length; i > cursor_position; i--){
				document[i] = document[i-1];
			}
			cursor_position++;
			document[cursor_position-1] = (char)ch;
			document[document_length] = '\0';
			unsaved = 1;
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
int render_cursor(){
	cursor_y = 0;
	cursor_x = 0;
	int line = 0;

	for (int i = 0; i < cursor_position; i++){//figure out how many newlines there are
		cursor_x++;
		if (document[i] == '\n' || cursor_x >= COLS-2){//detect when newline and also when text starts to wrap
			line++;
			if (line > line_offset){//find where the text shown on screen starts
				cursor_y++;
			}
			cursor_x = 0;
		}
	}
	mvwprintw(text,cursor_y,cursor_x,"\0");// null char does not show up so the cursor is moved here without pringing anything visible to the screen
	wrefresh(text);
}
void render_text(){
	int y = 0;
	int x = 0;
	int line = 0;
	int line_count;
	int extra_offset = 0;
	int negative_offset = 0; //POSITIVE number containing number of lines to be removed from the offset
	int i = 0;

	//check cursor position is on screen and if not ofset screen to ensure it is
	line_count = 0;
	for (i = 0; i < cursor_position; i++){
		if (document[i] == '\n' || x >= COLS-2){
			line++;
			x = 0;
			if (line <= line_offset){
				continue;
			}
			line_count++;
			if (line_count >= LINES-2){
				extra_offset++;
			}
		}
		x++;
	}
	negative_offset = line_offset-line;
	if (negative_offset < 0){
		negative_offset = 0;
	}
	//mvprintw(0,20,"negative_offset %d",negative_offset);
	line_offset += extra_offset - negative_offset;

	x = 0;
	line = 0;

	wclear(text);
	for (int i = 0; i < document_length; i++){
		if (document[i] == '\n'){//carrage return and newline when newline char found
			line++;
			if (line > line_offset){//dont newline if we havent reacher the offset yet
				y++;
				x = 0;
			}
			continue;
		}
		if (line >= line_offset){//dont print yet if we havent reached the desired offset
			if (x >= COLS-2){//detect if the text would go ofscreen and wrap it round
				y++;
				x = 0;
			}
			mvwprintw(text,y,x,"%c",document[i]);//print each char individualy
			x++;
		}
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
	if (unsaved){
		int selected = 0;
		int width = 35;
		int height = 4;
		int x = COLS/2 - (width/2);
		int y = LINES/2 - (height/2);
		int ch;
		WINDOW *popup;
		popup = newwin(height,width,y,x);//fixed size due to always displaying the same
		box(popup,0,0);
		mvwprintw(popup,1,2,"Quit? There are unsaved changes");
		mvwprintw(popup,2,8,">Cancel");mvwprintw(popup,2,19," Quit");
		curs_set(0);
		wrefresh(popup);
		while (1){
			ch = getch();
			if (ch == '\n'){
				break;
			}else if (ch == KEY_LEFT){
				selected = 0;
				mvwprintw(popup,2,8,">Cancel");mvwprintw(popup,2,19," Quit");
			}else if (ch == KEY_RIGHT){
				selected = 1;
				
				mvwprintw(popup,2,8," Cancel");mvwprintw(popup,2,19,">Quit");
			}
			wrefresh(popup);
		}
		curs_set(1);
		delwin(popup);
		render_screen();
		return selected;
	}else{
		return 1;
	}
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
	fclose(open_file);
	return 0;
}
int save_dialogue(){//return 0 for save and 1 for cancel
	char filename_buffer[MAX_BUFFER];
	char buffer[MAX_BUFFER];
	int selected = 0;
	int width = 35;
	int height = 4;
	int x = COLS/2 - (width/2);
	int y = LINES/2 - (height/2);
	int ch;
	
	snprintf(filename_buffer,MAX_BUFFER,"%s",filename);

	WINDOW *popup;
	popup = newwin(height,width,y,x);//fixed size due to always displaying the same
	box(popup,0,0);
	mvwprintw(popup,1,2,"Save as:");
	mvwprintw(popup,1,11,"%s",filename_buffer);
	mvwprintw(popup,2,2,">Save");mvwprintw(popup,2,8," Cancel");
	mvwprintw(popup,1,11+strlen(filename_buffer),"\0");
	wrefresh(popup);
	while (1){
		ch = getch();
		if (ch == '\n'){
			break;
		}else if (ch == KEY_LEFT){
			selected = 0;
			mvwprintw(popup,2,2,">Save");mvwprintw(popup,2,8," Cancel");
		}else if (ch == KEY_RIGHT){
			selected = 1;
			
			mvwprintw(popup,2,2," Save");mvwprintw(popup,2,8,">Cancel");
		}else if (ch == KEY_BACKSPACE){
			filename_buffer[strlen(filename_buffer)-1] = '\0';
			//clear deleted char from screen
			mvwprintw(popup,1,11+strlen(filename_buffer)," ");
		}else if(1){
			snprintf(buffer,MAX_BUFFER,"%s",filename_buffer);
			snprintf(filename_buffer,MAX_BUFFER,"%s%c",buffer,ch);
		}

		mvwprintw(popup,1,11,"%s",filename_buffer);
		//render cursor
		mvwprintw(popup,1,11+strlen(filename_buffer),"\0");

		wrefresh(popup);
	}
	delwin(popup);
	render_screen();
	snprintf(filename,MAX_BUFFER,"%s",filename_buffer);
	return selected;
}

int save_file(char *filename){
	char buffer;

	//preparing the file
	open_file = fopen(filename,"w");
	if (open_file == NULL){
		return 1;
	}

	//writing to it
	for (int i = 0;i<document_length;i++){
		buffer = document[document_length];
		fputc(buffer,open_file);
	}

	//cleanup
	fclose(open_file);
	return 0;
	unsaved = 0;
}
