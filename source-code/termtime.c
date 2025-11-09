#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <ncurses.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>

char *get_character(char c);

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
	//set the first alarm
	alarm(1);
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
		}else if (signal == SIGALRM){
			//update the time
			//schedule update for 1 second later
			alarm(1);
		}else if (signal == SIGINT){
			//exit
			break;
		}
		//====== redraw the clock ======
		//erase();
		refresh();
	}
	//====== shut everything down ======
	endwin();
	for (int i = 0; i < 10; i++) printf("%s\n",get_character('0'+i));
	return 0;
}

//get the ascii font for that specific character
char *get_character(char c){
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
