#include "../../source-code/framebuffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include <termios.h>

struct termios old_term_info;

void restore_terminal(){
	tcsetattr(STDIN_FILENO,0,&old_term_info);
	printf("\033[?25h");
}

int main(int argc, char **argv){
	//====== disable echo and cursor stdin ======
	//if (tcflow(STDIN_FILENO,TCOOFF) < 0){
	//	perror("tcflow");
	//	return 1;
	//}
	struct termios term_info;
	if (tcgetattr(STDIN_FILENO,&term_info) < 0){
		perror("tcgetattr");
		return 1;
	}
	memcpy(&old_term_info,&term_info,sizeof(struct termios));
	//add restore terminal hook
	atexit(restore_terminal);
	//turn off echo
	term_info.c_lflag &= ~(ECHO | ECHONL);
	if (tcsetattr(STDIN_FILENO,0,&term_info)){
		perror("tcsetattr");
		return 1;
	}
	//no cursor
	printf("\033[?25l");
	fflush(stdout);
	//====== startup framebuffer ======
	FRAMEBUFFER *frame_buffer = fb_new("/dev/fb0");
	if (frame_buffer == NULL){
		fprintf(stderr,"Could not initialise frame buffer\n");
		return 1;
	}
	size_t w = frame_buffer->width, h = frame_buffer->height;
	fb_fill(frame_buffer,rgb(0xff,0xff,0xff)); //fill with white
	fb_fill_area(frame_buffer,w/2-50,h/2-50,w/2+50,h/2+50,rgb(255,0,255));
	for (;;){
		fb_refresh(frame_buffer);
		sleep(1);
	}
	fb_free(frame_buffer);
	return 0;
}
