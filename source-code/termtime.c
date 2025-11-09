#include <stdio.h>
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

char *get_character_glyph(char c);
int draw_text(int y, int x, char *text);

int main(int argc, char **argv){
	//====== get ncurses set up ======
	setlocale(LC_ALL,"");
	initscr();
	noecho();
	curs_set(0);
	refresh();
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
	//enable signal drive io on stdin
	int stdin_flags = fcntl(STDIN_FILENO,F_GETFL);
	if (stdin_flags < 0){
		perror("fcntl(F_GETFL)");
		return 1;
	}
	if (fcntl(STDIN_FILENO,F_SETFL,stdin_flags | O_ASYNC) < 0){
		perror("fcntl(F_SETFL)");
		return 1;
	}
	//the shell probably owns signals from stdin
	//so we need to steal ownership
	if (fcntl(STDIN_FILENO,F_SETOWN,getpid()) < 0){
		perror("fcntl(F_SETOWN)");
		return 1;
	}
	//unbuffer stdin
	struct termios stdin_settings;
	if (tcgetattr(STDIN_FILENO,&stdin_settings) < 0){
		perror("tcgetattr");
		return 1;
	}
	stdin_settings.c_lflag &= ~(ICANON);
	stdin_settings.c_cc[VMIN] = 0;
	stdin_settings.c_cc[VTIME] = 0;
	if (tcsetattr(STDIN_FILENO,TCSANOW,&stdin_settings) < 0){
		perror("tcsetattr");
		return 1;
	}
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
			char input = 0;
			if (read(STDIN_FILENO,&input,1) < 0){
				perror("read");
				break;
			}
			if (input == 'q') break;
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
			"%H %M %S",
			time_info
		);
		//                   each letter is 10 chars wide (with one space at the end)
		int text_x = (COLS/2)-((timestring_bufferlen*(10+1))/2);
		//        each letter is 9 chars tall
		int text_y = (LINES/2) - 9/2;
		if (text_x < 0) text_x = 0;
		draw_text(text_y,text_x,timestring_buffer);
		refresh();
	}
	//====== shut everything down ======
	endwin();
	//for (int i = 0; i < 10; i++) printf("%s\n",get_character('0'+i));
	return 0;
}

int draw_text(int y, int x, char *text){
	mvprintw(y,x,"%s",text);
	//for each character in the string
	for (char *character = text; *character != '\0'; character++){
		//convert to glyph
		char *glyph = get_character_glyph(*character);
		//for each line in the glyph
		int current_y = 0;
		for (char *line = glyph; line-1 != NULL; line = strchr(line,'\n')+1){
			long line_length = (strchr(line,'\n') != NULL) ? (strchr(line,'\n')-line) : strlen(line);
			mvaddnstr(y+current_y,x,line,line_length);
			current_y++;
		}
		//move cursor across
		x += 11;
	}
}

//get the ascii font for that specific character
char *get_character_glyph(char c){
	if (c >= '0' && c <= '9'){
		return (char *[]){
			"\
  <====>\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>",
			"\
        \n\
        /\\\n\
        ||\n\
        \\/\n\
        \n\
        /\\\n\
        ||\n\
        \\/\n\
        ",
			"\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
  <====>\n\
/\\\n\
||\n\
\\/ \n\
  <====>",
			"\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
  <====>",
			"\
\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
",
			"\
  <====>\n\
/\\\n\
||\n\
\\/\n\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
  <====>",
			"\
  <====>\n\
/\\\n\
||\n\
\\/\n\
  <====>\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>",
			"\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
\n\
        /\\\n\
        ||\n\
        \\/\n\
",
			"\
  <====>\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>",
			"\
  <====>\n\
/\\      /\\\n\
||      ||\n\
\\/      \\/\n\
  <====>\n\
        /\\\n\
        ||\n\
        \\/\n\
  <====>",
		}[c-'0'];
	}else return "";
}
