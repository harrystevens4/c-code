#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <curses.h> //for the ACS_VLINE defs and such
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
void alpha_sort(char **list,int length);
void exec_cmd(const char *command,char **output);
void popup(const char *message);
int main(){
	/* initialisation of ncurses */
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
			if (dir->d_type == DT_REG){//files only
				count++;
				files = realloc(files,sizeof(char*)*count);
				files[count-1] = malloc(sizeof(char)*(strlen(dir->d_name)+1));
				strcpy(files[count-1],dir->d_name);
			}
		}
		closedir(cur_dir);
		/* sort alphabeticaly */
		alpha_sort(files,count);
	}else{
		return 1;
	}
	const int term_width = COLS;
	const int term_height = LINES;
	const int max_height = 15;
	const int min_height = 10;
	const int max_width = 50;
	const int min_width = 15;
	int width;
	int height;
	char *result; //storing the output from commands
	char buffer[1024]; //cant be bother to dynamicaly alocate (for the concatinated command)
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
	int status; //return status from commands
	const int number_of_actions = 5;
	const int action_width = 13; //width of the right side actions
	const int selection_width = width - action_width - 3;
	const char actions[5][15] = {"<status>","<start>","<stop>","<enable>","<disable>"};//right side action menu options
	int character=0;
	char letter;
	int ellipses = 0;
	int mid_x = x+width-action_width; //x of mid partition
	refresh();

	//cleanup whatever was on screen previously
	clear();
	/* initialise the window */
	WINDOW *win;
	win = newwin(height,width,y,x);
	refresh();

	//put new window to screen
	refresh();

	while (!exit){
		/* redraw */	
		box(win,0,0);
		mvvline(y+1,mid_x,ACS_VLINE,height-2);//centre partition
		wrefresh(win);

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
		/* push to screen */
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
			case 10: //enter
				 switch (action){
					case 0://status
						sprintf(buffer,"/bin/systemctl status %s",files[selected]);
						exec_cmd((const char *)buffer,&result);
						popup((const char *)result);
						
						break;
					case 1://start
						sprintf(buffer,"/bin/systemctl start %s",files[selected]);
						status = system(buffer);
						if (status){
							popup("failure :(");
						}else{
							popup("success!");
						}
						break;
					case 2://stop
						sprintf(buffer,"/bin/systemctl stop %s",files[selected]);
						status = system(buffer);
						if (status){
							popup("failure :(");
						}else{
							popup("success!");
						}
						break;
					case 3://enable
						sprintf(buffer,"/bin/systemctl enable %s",files[selected]);
						status = system(buffer);
						if (status){
							popup("failure :(");
						}else{
							popup("success!");
						}
						break;
					case 4://disable
						sprintf(buffer,"/bin/systemctl disable %s",files[selected]);
						status = system(buffer);
						if (status){
							popup("failure :(");
						}else{
							popup("success!");
						}
						break;

				}
				//exit=1;
				break;
			case KEY_UP:
				if (selected > 0){
					selected--;
				}
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
		printf("[%s]\n",files[i]);
		free(files[i]);
	}
	free(files);

	printf("finnished successfully\n");
	printf("term height %d term width %d height %d width %d\n",term_height,term_width,height,width);
	printf("%d\n",input);
	exec_cmd("/bin/ls /home/harry/cbin",&result);
	printf("%s",result);
	free(result);
	return 0;
}
void alpha_sort(char **list,int length){
	int sorted = 0;
	char *temp;
	for (;;){
		sorted=1;
		for (int i=0;i<length-1;i++){
			if (strcmp(list[i],list[i+1])>0){
				temp = list[i];
				list[i] = list[i+1];
				list[i+1] = temp;
				sorted=0;
			}
		}
		if (sorted){
			break;
		}
	}
}
void popup(const char *message){
	/* initialise variables */
	int height = 3;
	int width = strlen(message)+4;//1 space padding each side
	int offset = 0;//newline offset
	int shift = 0; //character offset

	/* ajust window size to fit string*/
	int i;
	int max_length = 0;
	int line_length = 1;
	int message_height = 1;
	for (i=0;;i++){
		if (message[i]=='\0'){
			break;
		}
		if (message[i]=='\n'){
			message_height++;
			line_length=1;
		}
		if (line_length>max_length){
			max_length=line_length;
		}
		
		line_length++;
	}
	height += message_height;
	width = max_length+4;
	if (width < 8){
		width = 8; //enough space for the " <ok> "
	}
	if (width > COLS){
		width = COLS;
	}
	int message_length = i;
	int x = (COLS/2)-2-(width/2);
	if (x<0){
		x=0;
	}
	int y = (LINES/2)-(height/2);

	/* create popup window */
	WINDOW *popup_win;
	popup_win = newwin(height,width,y,x);
	refresh();
	box(popup_win,0,0);
	wrefresh(popup_win);
	for (i=0;i<message_length;i++){
		if (message[i] == '\n'){
			offset++;
			shift=0;
			continue;
		}
		mvprintw(y+1+offset,x+2+shift,"%c",message[i]);
		shift++;
	}
	attrset(COLOR_PAIR(2));
	mvprintw(y+height-2,x+(int)(width/2)-3," <ok> ");
	attrset(COLOR_PAIR(1));
	refresh();
	/* wait for input to close window */
	getch();
	/* return screen to og state */
	delwin(popup_win);
	clear();
	refresh();
}
void exec_cmd(const char *command,char **output){
	*output = malloc(sizeof(char));
	FILE *file_pointer;
	char path[1035];
	int count = 0;

	file_pointer = popen(command,"r");
	if (file_pointer == NULL){
		printf("could not open file\n");
		return;
	}
	while (fgets(path, sizeof(path), file_pointer) != NULL){
		for (int character = 0;;character++){
			*output = realloc(*output,sizeof(char)*(count+1));
			if (path[character] == '\0'){
				break;
			}else{
				output[0][count] = path[character];
			}
			count++;
		}
	}
	pclose(file_pointer);
}

