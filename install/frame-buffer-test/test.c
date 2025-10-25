#include "../../source-code/framebuffer.h"
#include <poll.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include <termios.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

struct termios old_term_info;

void restore_terminal(){
	tcsetattr(STDIN_FILENO,0,&old_term_info);
	printf("\033[?25h");
}

int main(int argc, char **argv){
	//====== configuration ======
	float sensitivity = 4.0;
	//====== disable echo and cursor stdin ======
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
	//====== open the mouse ======
	int mouse_fd = open("/dev/input/mice",O_RDONLY); //uses PS/2 interface
	if (mouse_fd < 0){
		perror("open");
		return 1;
	}
	//====== startup framebuffer ======
	FRAMEBUFFER *frame_buffer = fb_new("/dev/fb0");
	if (frame_buffer == NULL){
		fprintf(stderr,"Could not initialise frame buffer\n");
		close(mouse_fd);
		return 1;
	}
	size_t w = frame_buffer->width, h = frame_buffer->height;
	fb_fill(frame_buffer,rgb(0xff,0xff,0xff)); //fill with white
	fb_refresh(frame_buffer);
	long mouse_x = 0;
	long mouse_y = 0;
	for (;;){
		//====== await mouse input ======
		//struct pollfd pollfds[1] = {
		//	{ .fd = mouse_fd, .events = POLLOUT },
		//};
		//if (poll(pollfds,1,0) < 0){
		//	perror("poll");
		//	break;
		//}
		char event_buffer[3];
		if (read(mouse_fd,&event_buffer,sizeof(event_buffer)) < 0){
			perror("read");
			break;
		}
		int x_movement = (float)event_buffer[1]*sensitivity, y_movement = -(float)event_buffer[2]*sensitivity;
		mouse_x = MAX(MIN(mouse_x+x_movement,(long)w),0);
		mouse_y = MAX(MIN(mouse_y+y_movement,(long)h),0);
		//printf("%ld,%ld\n",mouse_x,mouse_y);
		//====== render a cursor ======
		fb_fill(frame_buffer,rgb(0xff,0xff,0xff)); //fill with white
		fb_fill_area(frame_buffer,mouse_x-10,mouse_y-10,mouse_x+10,mouse_y+10,rgb(66,135,245));
		fb_refresh(frame_buffer);
	}
	fb_free(frame_buffer);
	close(mouse_fd);
	return 0;
}
