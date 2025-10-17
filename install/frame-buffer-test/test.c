#include "../../source-code/framebuffer.h"
#include <stdio.h>

int main(int argc, char **argv){
	FRAMEBUFFER *frame_buffer = fb_new("/dev/fb0");
	if (frame_buffer == NULL){
		fprintf(stderr,"Could not initialise frame buffer\n");
		return 1;
	}
	fb_fill(frame_buffer,0x0000ff); //fill with blue
	fb_refresh(frame_buffer);
	fb_free(frame_buffer);
	return 0;
}
