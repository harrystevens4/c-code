#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <pthread.h>
#include <stdint.h>
#include <stddef.h>
#include <linux/fb.h>

//====== structures ======
struct frame_buffer_layer {
	enum {
		FB_LAYER_RECT,
	} type;
};
struct frame_buffer {
	int fd;
	size_t width;
	size_t height;
	pthread_mutex_t buffer_mutex;
	uint32_t *buffer; //array of all pixel information stored as rgba with 8 bits for each colour
	/*treated as struct __attribute__((__packed__)) {
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t transp;
	}*/
	size_t buffer_len;
	struct fb_var_screeninfo screeninfo;
};
typedef struct frame_buffer FRAMEBUFFER;

//====== functions ======
FRAMEBUFFER *fb_new(const char *path);
void fb_free(FRAMEBUFFER *fb);
void fb_fill(FRAMEBUFFER *fb, uint32_t colour);
int fb_refresh(FRAMEBUFFER *fb);
uint32_t rgb(uint8_t red, uint8_t green, uint8_t blue);
void fb_fill_area(FRAMEBUFFER *fb, size_t x1, size_t y1, size_t x2, size_t y2, uint32_t colour);

#endif
