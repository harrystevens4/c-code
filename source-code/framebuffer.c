#include <linux/fb.h>
#include <endian.h>
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
	fb->fd = fbfd;
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
int fb_refresh(FRAMEBUFFER *fb){
	//jumps to kernel mode are slow so lets only do it once
	uint32_t *new_frame_buffer = malloc(fb->buffer_len*(fb->screeninfo.bits_per_pixel/8));
	size_t new_frame_buffer_len = fb->buffer_len*(fb->screeninfo.bits_per_pixel/8);
	memset(new_frame_buffer,0,new_frame_buffer_len);
	//====== copy all the pixels over to the real frame buffer ======
	for (size_t i = 0; i < fb->buffer_len; i++){
		//we need to do colour translation
		//int bpp = fb->screeninfo.bits_per_pixel;
		//convert to fixed endianness 
		uint32_t buffer_pixel = fb->buffer[i];
		uint32_t red = ((uint8_t *)&buffer_pixel)[0];
		uint32_t green = ((uint8_t *)&buffer_pixel)[1];
		uint32_t blue = ((uint8_t *)&buffer_pixel)[2];

		//int red_length = fb->screeninfo.red.length;
		//int green_length = fb->screeninfo.green.length;
		//int blue_length = fb->screeninfo.blue.length;
		int red_offset = fb->screeninfo.red.offset;
		int green_offset = fb->screeninfo.green.offset;
		int blue_offset = fb->screeninfo.blue.offset;

		uint32_t pixel = 0
			| (red << (red_offset))
			| (green << (green_offset))
			| (blue << (blue_offset));
		new_frame_buffer[i] = pixel;
	}
	int result = pwrite(fb->fd,new_frame_buffer,new_frame_buffer_len,0);
	if (result < (long int)new_frame_buffer_len){
		perror("write");
		return -1;
	}
	free(new_frame_buffer);
	return 0;
}
uint32_t rgb(uint8_t red, uint8_t green, uint8_t blue){
	uint8_t colours[4] = {0};
	colours[0] = red;
	colours[1] = green;
	colours[2] = blue;
	uint32_t *colour = (uint32_t *)colours;
	return *colour;
}
void fb_fill(FRAMEBUFFER *fb, uint32_t colour){
	for (size_t i = 0; i < fb->width * fb->height; i++) fb->buffer[i] = colour;
}
void fb_fill_area(FRAMEBUFFER *fb, size_t x1, size_t y1, size_t x2, size_t y2, uint32_t colour){
	for (size_t y = 0; y < fb->height; y++){
		for (size_t x = 0; x < fb->width; x++){
			if (x >= x1 && x <= x2 && y >= y1 && y <= y2){
				fb->buffer[y*(fb->width)+x] = colour;
			}
		}
	}
}
