#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

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
	uint32_t *buffer; //array of all pixel information stored as rgba with 8 bits for each colour
	size_t buffer_len;
	struct fb_var_screeninfo screeninfo;
};
typedef struct frame_buffer FRAMEBUFFER;

//====== functions ======
FRAMEBUFFER *fb_new(const char *path);
void fb_free(FRAMEBUFFER *fb);
void fb_fill(FRAMEBUFFER *fb, uint32_t colour);
void fb_refresh(FRAMEBUFFER *fb);

#endif
