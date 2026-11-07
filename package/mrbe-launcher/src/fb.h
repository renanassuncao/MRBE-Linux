#ifndef MRBE_FB_H
#define MRBE_FB_H

#include <linux/fb.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    int fd;
    uint8_t *memory;      /* drawing target, first visible pixel */
    size_t memory_size;
    uint8_t *map;         /* device mapping; NULL in the host preview */
    size_t map_size;
    uint8_t *screen;      /* first visible pixel inside map */
    int bytes_per_pixel;
    struct fb_fix_screeninfo fixed;
    struct fb_var_screeninfo variable;
} MrbeFramebuffer;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} MrbeColor;

typedef struct {
    uint8_t *pixels; /* RGBA8888 */
    int width;
    int height;
} MrbeImage;

int mrbe_fb_open(MrbeFramebuffer *fb, const char *device);
void mrbe_fb_close(MrbeFramebuffer *fb);
void mrbe_fb_present(MrbeFramebuffer *fb);
void mrbe_fb_clear(MrbeFramebuffer *fb, MrbeColor color);
void mrbe_fb_fill_rect(MrbeFramebuffer *fb, int x, int y, int width, int height, MrbeColor color);
void mrbe_fb_rect(MrbeFramebuffer *fb, int x, int y, int width, int height, int thickness, MrbeColor color);
void mrbe_fb_text(MrbeFramebuffer *fb, int x, int y, int scale, MrbeColor color, const char *text);
void mrbe_fb_text_center(MrbeFramebuffer *fb, int center_x, int y, int scale, MrbeColor color, const char *text);
int mrbe_fb_load_rgba(MrbeImage *image, const char *path, int width, int height);
void mrbe_fb_draw_rgba(MrbeFramebuffer *fb, const MrbeImage *image,
                       int x, int y, int width, int height, MrbeColor under);
void mrbe_fb_free_image(MrbeImage *image);

#endif
