#include <stdio.h>
#include <getopt.h>
#include <curses.h>
#include <wchar.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <ncurses.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>

int setup_stdin();
int get_text_width(char *text);
wchar_t *get_character_glyph(char c);
int draw_text(int y, int x, char *text);
void print_help(char *name);
time_t prompt_for_timer_length();

int main(int argc, char **argv){
	char *clock_format_string = "%H:%M:%S";
	int timer_mode = 0;
	time_t timer_end_time = 0;
	//====== process command line arguments ======
	struct option long_options[] = {
		{"help",no_argument,0,'h'},
		{"format",required_argument,0,'f'},
		{"timer",no_argument,0,'t'},
	};
	for (;;){
		int option_index = 0;
		int c = getopt_long(argc,argv,"hf:t",long_options,&option_index);
		if (c == -1) break;
		switch (c){
		case 'h':
			print_help(argv[0]);
			return 0;
		case 'f':
			clock_format_string = optarg;
			break;
		case 't':
			timer_mode = 1;
		}

	}
	//====== get ncurses set up ======
	setlocale(LC_ALL,"");
	initscr();
	noecho();
	curs_set(0);
	keypad(stdscr,true);
	refresh();
	//====== initialise the timer if requested ======
	if (timer_mode) timer_end_time = prompt_for_timer_length() + time(NULL);
	//====== block SIGIO, SIGALRM, SIGINT and SIGWINCH  ======
	//that way we can be completely signal driven
	//so we yeild to other processes when nothing is happening
	sigset_t blocked_signals;
	sigemptyset(&blocked_signals);
	sigaddset(&blocked_signals,SIGWINCH);
	sigaddset(&blocked_signals,SIGALRM);
	sigaddset(&blocked_signals,SIGINT);
	sigaddset(&blocked_signals,SIGIO);
	sigprocmask(SIG_BLOCK,&blocked_signals,NULL);
	//get stdin ready for us
	setup_stdin();
	//start the alarm loop (alarm() cannot create a 0 second alarm)
	kill(getpid(),SIGALRM);
	//====== update loop ======
	for (;;){
		//====== wait for signal ======
		int signal = 0;
		sigwait(&blocked_signals,&signal);
		//====== handle the signal ======
		if (signal == SIGWINCH){
			//get new terminal size
			struct winsize window_size;
			if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&window_size) < 0){
				perror("ioctl");
				break;
			}
			resizeterm(window_size.ws_row,window_size.ws_col);
		}else if (signal == SIGIO){
			//keypress
			char input[1024] = {0}; 
			//read only one byte and discard the rest
			if (read(STDIN_FILENO,input,1) < 0){
				perror("read");
				break;
			}
			if (*input == 'q') break;
			//no need to re render the clock
			continue;
		}else if (signal == SIGALRM){
			//update the time
			//schedule update for 1 second later
			alarm(1);
		}else if (signal == SIGINT){
			//exit
			break;
		}
		//====== get the time ======
		time_t timestamp = time(NULL);
		struct tm *time_info = localtime(&timestamp);
		//====== redraw the clock ======
		erase();
		//terminal too small?
		if (LINES < 9){
			printw("Please increase the size of the window");
			refresh();
			continue;
		}
		//render the text
		char timestring_buffer[1024] = {0};
		size_t timestring_bufferlen = strftime(
			timestring_buffer,
			sizeof(timestring_buffer),
			clock_format_string,
			time_info
		);
		if (timestring_bufferlen == 0){
			timestring_buffer[0] = '\0';
		}
		//centring the text
		int text_x = (COLS/2)-(get_text_width(timestring_buffer)/2);
		//        each letter is 9 chars tall
		int text_y = (LINES/2) - 9/2;
		if (text_x < 0) text_x = 0;
		draw_text(text_y,text_x,timestring_buffer);
		refresh();
	}
	//====== shut everything down ======
	endwin();
	return 0;
}

void print_help(char *name){
	printf("usage: %s [options]\n",name);
	printf("options:\n");
	printf("	-h, --help       : print help text\n");
	printf("	-f, --format <s> : use s as the strftime format string\n");
}

int setup_stdin(){
	//====== unbuffer stdin ======
	struct termios stdin_settings;
	if (tcgetattr(STDIN_FILENO,&stdin_settings) < 0){
		perror("tcgetattr");
		return -1;
	}
	stdin_settings.c_lflag &= ~(ICANON);
	stdin_settings.c_cc[VMIN] = 0;
	stdin_settings.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO,TCSANOW,&stdin_settings) < 0){
		perror("tcsetattr");
		return -1;
	}
	//====== enable async mode ======
	int stdin_flags = fcntl(STDIN_FILENO,F_GETFL);
	if (stdin_flags < 0){
		perror("fcntl(F_GETFL)");
		return -1;
	}
	//whenever input becomes available the kernel will send a SIGIO
	if (fcntl(STDIN_FILENO,F_SETFL,stdin_flags | O_ASYNC) < 0){
		perror("fcntl(F_SETFL)");
		return -1;
	}
	//the shell probably owns signals from stdin
	//so we need to steal ownership
	if (fcntl(STDIN_FILENO,F_SETOWN,getpid()) < 0){
		perror("fcntl(F_SETOWN)");
		return -1;
	}
	return 0;
}

time_t prompt_for_timer_length(){
	time_t seconds = 0;
	time_t minutes = 0;
	time_t hours = 0;
	int selected_interval = 0;
	const int time_interval_spacing = 6;
	for (;;){
		//====== render the currently selected time ======
		erase();
		char hours_text[128] = {0};
		char minutes_text[128] = {0};
		char seconds_text[128] = {0};
		snprintf(hours_text,sizeof(hours_text),"%.2lu",hours);
		snprintf(minutes_text,sizeof(minutes_text),"%.2lu",minutes);
		snprintf(seconds_text,sizeof(seconds_text),"%.2lu",seconds);
		size_t hours_width = get_text_width(hours_text);
		size_t minutes_width = get_text_width(minutes_text);
		size_t seconds_width = get_text_width(seconds_text);
		size_t total_width = hours_width + minutes_width + seconds_width + time_interval_spacing*2;
		//centreing
		int x = (COLS/2)-(total_width/2);
		int y = (LINES/2)-5;
		//the numbers
		draw_text(y+1,x,hours_text);
		draw_text(y+1,x+hours_width+time_interval_spacing,minutes_text);
		draw_text(y+1,x+hours_width+minutes_width+(time_interval_spacing*2),seconds_text);
		//labels for "hours", "minutes", and "seconds"
		mvprintw(y-1,x,"Hours");
		mvprintw(y-1,x+hours_width+time_interval_spacing,"Minutes");
		mvprintw(y-1,x+hours_width+minutes_width+(time_interval_spacing*2),"Seconds");
		//====== remder the selected interval indicator ======
		switch (selected_interval){
			case 0:
				mvprintw(y,x+(hours_width/2)-4,"/\\ /\\ /\\");
				mvprintw(y+10,x+(hours_width/2)-4,"\\/ \\/ \\/");
				break;
			case 1:
				mvprintw(y,x+(minutes_width/2)-4+hours_width+time_interval_spacing,"/\\ /\\ /\\");
				mvprintw(y+10,x+(minutes_width/2)-4+hours_width+time_interval_spacing,"\\/ \\/ \\/");
				break;
			case 2:
				mvprintw(y,x+(seconds_width/2)-4+hours_width+minutes_width+(time_interval_spacing*2),"/\\ /\\ /\\");
				mvprintw(y+10,x+(seconds_width/2)-4+hours_width+minutes_width+(time_interval_spacing*2),"\\/ \\/ \\/");
				break;
		}
		mvprintw(0,0,"%d",selected_interval);
		refresh();
		//====== await user input ======
		int input = getch();
		switch (input){
		case '\n':
			return seconds + minutes*60 + hours*3600;
		case KEY_LEFT:
			selected_interval = (selected_interval > 0) ? (selected_interval - 1) : 2;
			break;
		case KEY_RIGHT:
			selected_interval = (selected_interval + 1) % 3;
			break;
		case KEY_UP:
			if (selected_interval == 0) hours++;
			if (selected_interval == 1) minutes = (minutes + 1)%60;
			if (selected_interval == 2) seconds = (seconds + 1)%60;
			break;
		case KEY_DOWN:
			if (selected_interval == 0) hours = (hours > 0) ? hours-1 : 0;
			if (selected_interval == 1) minutes = (minutes > 0) ? minutes-1 : 59;
			if (selected_interval == 2) seconds = (seconds > 0) ? seconds-1 : 59;
			break;
		}
	}
}

int get_text_width(char *text){
	size_t x = 0;
	for (char *character = text; *character != '\0'; character++){
		//convert to glyph
		size_t max_line_len = 0;
		wchar_t *glyph = get_character_glyph(*character);
		//for each line in the glyph
		for (wchar_t *line = glyph;;){
			long line_length = (wcschr(line,L'\n') != NULL) ? (unsigned long)(wcschr(line,L'\n')-line) : (unsigned long)wcslen(line);
			if (line_length > (long)max_line_len) max_line_len = line_length;
			line = wcschr(line,L'\n');
			if (line == NULL) break;
			line++;
		}
		//move cursor across
		x += max_line_len + 1;
	}
	return x;
}

int draw_text(int y, int x, char *text){
	//for each character in the string
	for (char *character = text; *character != L'\0'; character++){
		//convert to glyph
		size_t max_line_len = 0;
		wchar_t *glyph = get_character_glyph(*character);
		//for each line in the glyph
		int current_y = 0;
		for (wchar_t *line = glyph;;){
			long line_length = (wcschr(line,L'\n') != NULL) ? (unsigned long)(wcschr(line,L'\n')-line) : (unsigned long)wcslen(line);
			mvaddnwstr(y+current_y,x,line,line_length);
			if (line_length > (long)max_line_len) max_line_len = line_length;
			current_y++;
			line = wcschr(line,L'\n');
			if (line == NULL) break;
			line++;
		}
		//move cursor across
		x += max_line_len + 1;
	}
	return 0;
}

//get the ascii font for that specific character
wchar_t *get_character_glyph(char c){
	//====== colon ======
	switch (c){
	case ':':
			return L"\
        \n\
   ██   \n\
        \n\
        \n\
        \n\
        \n\
        \n\
   ██   \n\
        \n\
        ";
	case 'a':
		return L"\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
        ";
	case 'p':
		return L"\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ \n\
█       \n\
█       \n\
█       \n\
        ";
	case 'm':
		return L"\
 ███ ███ \n\
█   █   █\n\
█   █   █\n\
█   █   █\n\
         \n\
█   █   █\n\
█   █   █\n\
█   █   █\n\
        ";
	}
	//====== numbers ======
	if (c >= '0' && c <= '9'){
		return (wchar_t *[]){
			L"\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
         \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ ",
			L"\
         \n\
        █\n\
        █\n\
        █\n\
         \n\
        █\n\
        █\n\
        █\n\
         ",
			L"\
 ███████ \n\
        █\n\
        █\n\
        █\n\
 ███████ \n\
█        \n\
█        \n\
█        \n\
 ███████ ",
			L"\
 ███████ \n\
        █\n\
        █\n\
        █\n\
 ███████ \n\
        █\n\
        █\n\
        █\n\
 ███████ ",
			L"\
         \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ \n\
        █\n\
        █\n\
        █\n\
         ",
			L"\
 ███████ \n\
█        \n\
█        \n\
█        \n\
 ███████ \n\
        █\n\
        █\n\
        █\n\
 ███████ ",
			L"\
 ███████ \n\
█        \n\
█        \n\
█        \n\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ ",
			L"\
 ███████ \n\
        █\n\
        █\n\
        █\n\
         \n\
        █\n\
        █\n\
        █\n\
         ",
			L"\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ ",
			L"\
 ███████ \n\
█       █\n\
█       █\n\
█       █\n\
 ███████ \n\
        █\n\
        █\n\
        █\n\
         ",
		}[c-'0'];
	}else return L"       ";
}
