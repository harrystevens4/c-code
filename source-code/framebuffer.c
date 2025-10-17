#include <linux/fb.h>
#include "framebuffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

FRAMEBUFFER *fb_new(const char *path){
	//====== initialise fb struct ======
	FRAMEBUFFER *fb = malloc(sizeof(FRAMEBUFFER));
	memset(fb,0,sizeof(FRAMEBUFFER));
	//====== get fb information ======
	int fbfd = open(path,O_RDWR);
	if (fbfd < 0){
		perror("read");
		fb_free(fb);
		return NULL;
	}
	struct fb_fix_screeninfo f_info = {0};
	struct fb_var_screeninfo v_info = {0};
	if (ioctl(fbfd,FBIOGET_FSCREENINFO,&f_info) < 0){
		perror("ioctl");
		fb_free(fb);
		return NULL;
	}
	if (ioctl(fbfd,FBIOGET_VSCREENINFO,&v_info) < 0){
		perror("ioctl");
		fb_free(fb);
		return NULL;
	}
	size_t width = v_info.xres, height = v_info.yres;
	fb->width = width; fb->height = height;
	fb->buffer_len = width*height;
	memcpy(&fb->screeninfo,&v_info,sizeof(struct fb_var_screeninfo));
	//====== initialise the buffer ======
	fb->buffer = calloc(width*height,sizeof(uint32_t));
	return fb;
}
void fb_free(FRAMEBUFFER *fb){
	close(fb->fd);
	free(fb->buffer);
	free(fb);
}
void fb_refresh(FRAMEBUFFER *fb){
	//jumps to kernel mode are slow so lets only do it once
	uint32_t *new_frame_buffer = calloc(fb->buffer_len,fb->screeninfo.bits_per_pixel/8);
	size_t new_frame_buffer_len = fb->buffer_len*(fb->screeninfo.bits_per_pixel/8);
	//====== copy all the pixels over to the real frame buffer ======
	for (size_t i = 0; i < fb->buffer_len; i++){
		//we need to do colour translation
		int bpp = fb->screeninfo.bits_per_pixel;
		uint32_t red = ((uint8_t *)(fb->buffer+i))[0];
		uint32_t green = ((uint8_t *)(fb->buffer+i))[1];
		uint32_t blue = ((uint8_t *)(fb->buffer+i))[2];
		uint32_t transp = ((uint8_t *)(fb->buffer+i))[3];

		int red_length = fb->screeninfo.red.length;
		int green_length = fb->screeninfo.green.length;
		int blue_length = fb->screeninfo.blue.length;
		int transp_length = fb->screeninfo.transp.length;
		int red_offset = fb->screeninfo.red.offset;
		int green_offset = fb->screeninfo.green.offset;
		int blue_offset = fb->screeninfo.blue.offset;
		int transp_offset = fb->screeninfo.transp.offset;

		uint32_t pixel = 0
			| (red << (bpp - red_offset - red_length))
			| (green << (bpp - green_offset - green_length))
			| (blue << (bpp - blue_offset - blue_length))
			| (transp << (bpp - transp_offset - transp_length));
		new_frame_buffer[i] = pixel;
	}
	pwrite(fb->fd,new_frame_buffer,new_frame_buffer_len,0);
	free(new_frame_buffer);
}
void fb_fill(FRAMEBUFFER *fb, uint32_t colour){
	for (size_t i = 0; i < fb->width * fb->height; i++) fb->buffer[i] = colour;
}
