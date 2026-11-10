#include "fb.h"
#include "font.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static uint32_t mrbe_component(uint8_t value, struct fb_bitfield field)
{
    uint32_t maximum;
    if (field.length == 0) return 0;
    if (field.length >= 32) maximum = 0xFFFFFFFFu;
    else maximum = (1u << field.length) - 1u;
    return (((uint32_t)value * maximum / 255u) & maximum) << field.offset;
}


int mrbe_fb_open(MrbeFramebuffer *fb, const char *device)
{
    if (!fb || !device) return -1;
    memset(fb, 0, sizeof(*fb));
    fb->fd = -1;

    fb->fd = open(device, O_RDWR);
    if (fb->fd < 0) {
        perror("open framebuffer");
        return -1;
    }
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fixed) < 0 ||
        ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->variable) < 0) {
        perror("framebuffer ioctl");
        mrbe_fb_close(fb);
        return -1;
    }

    fb->bytes_per_pixel = (int)((fb->variable.bits_per_pixel + 7u) / 8u);
    if (fb->bytes_per_pixel < 2 || fb->bytes_per_pixel > 4) {
        fprintf(stderr, "Unsupported framebuffer depth: %u bpp\n",
                fb->variable.bits_per_pixel);
        mrbe_fb_close(fb);
        return -1;
    }

    fb->memory_size = fb->fixed.smem_len;
    fb->memory = (uint8_t *)mmap(NULL, fb->memory_size, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, fb->fd, 0);
    if (fb->memory == MAP_FAILED) {
        fb->memory = NULL;
        perror("mmap framebuffer");
        mrbe_fb_close(fb);
        return -1;
    }

    fprintf(stderr, "MRBE framebuffer: %ux%u, %u bpp\n",
            fb->variable.xres, fb->variable.yres, fb->variable.bits_per_pixel);
    return 0;
}

void mrbe_fb_close(MrbeFramebuffer *fb)
{
    if (!fb) return;
    if (fb->memory) munmap(fb->memory, fb->memory_size);
    if (fb->fd >= 0) close(fb->fd);
    fb->memory = NULL;
    fb->fd = -1;
}

void mrbe_fb_fill_rect(MrbeFramebuffer *fb, int x, int y, int width, int height, MrbeColor color)
{
    int x0, y0, x1, y1, yy;
    uint32_t pixel;

    if (!fb || !fb->memory || width <= 0 || height <= 0) return;

    /* Clip once, then use a tight row loop. This is substantially faster than
       calling mrbe_pixel() for every pixel when the TV launcher redraws a frame. */
    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + width;
    y1 = y + height;
    if (x1 > (int)fb->variable.xres) x1 = (int)fb->variable.xres;
    if (y1 > (int)fb->variable.yres) y1 = (int)fb->variable.yres;
    if (x0 >= x1 || y0 >= y1) return;

    pixel = mrbe_component(color.r, fb->variable.red) |
            mrbe_component(color.g, fb->variable.green) |
            mrbe_component(color.b, fb->variable.blue);

    for (yy = y0; yy < y1; ++yy) {
        size_t offset = (size_t)(yy + (int)fb->variable.yoffset) *
                        (size_t)fb->fixed.line_length +
                        (size_t)(x0 + (int)fb->variable.xoffset) *
                        (size_t)fb->bytes_per_pixel;
        uint8_t *row;
        int xx;
        if (offset >= fb->memory_size) continue;
        row = fb->memory + offset;

        if (fb->bytes_per_pixel == 4) {
            uint32_t *dst = (uint32_t *)row;
            for (xx = x0; xx < x1; ++xx) *dst++ = pixel;
        } else if (fb->bytes_per_pixel == 2) {
            uint16_t value = (uint16_t)pixel;
            uint16_t *dst = (uint16_t *)row;
            for (xx = x0; xx < x1; ++xx) *dst++ = value;
        } else {
            for (xx = x0; xx < x1; ++xx) {
                row[0] = (uint8_t)(pixel & 0xffu);
                row[1] = (uint8_t)((pixel >> 8) & 0xffu);
                row[2] = (uint8_t)((pixel >> 16) & 0xffu);
                row += 3;
            }
        }
    }
}

void mrbe_fb_clear(MrbeFramebuffer *fb, MrbeColor color)
{
    if (!fb) return;
    mrbe_fb_fill_rect(fb, 0, 0, (int)fb->variable.xres, (int)fb->variable.yres, color);
}

void mrbe_fb_rect(MrbeFramebuffer *fb, int x, int y, int width, int height, int thickness, MrbeColor color)
{
    int i;
    if (thickness < 1) thickness = 1;
    for (i = 0; i < thickness; ++i) {
        mrbe_fb_fill_rect(fb, x + i, y + i, width - 2 * i, 1, color);
        mrbe_fb_fill_rect(fb, x + i, y + height - 1 - i, width - 2 * i, 1, color);
        mrbe_fb_fill_rect(fb, x + i, y + i, 1, height - 2 * i, color);
        mrbe_fb_fill_rect(fb, x + width - 1 - i, y + i, 1, height - 2 * i, color);
    }
}

void mrbe_fb_text(MrbeFramebuffer *fb, int x, int y, int scale, MrbeColor color, const char *text)
{
    int cursor;
    if (!fb || !text || scale < 1) return;
    cursor = x;
    while (*text) {
        const uint8_t *glyph = mrbe_font_glyph(*text++);
        int col, row;
        for (col = 0; col < 5; ++col) {
            for (row = 0; row < 7; ++row) {
                if (glyph[col] & (1u << row))
                    mrbe_fb_fill_rect(fb, cursor + col * scale, y + row * scale,
                                      scale, scale, color);
            }
        }
        cursor += 6 * scale;
    }
}

void mrbe_fb_text_center(MrbeFramebuffer *fb, int center_x, int y, int scale, MrbeColor color, const char *text)
{
    size_t length;
    int width;
    if (!text) return;
    length = strlen(text);
    width = (int)length * 6 * scale;
    if (length > 0) width -= scale;
    mrbe_fb_text(fb, center_x - width / 2, y, scale, color, text);
}


int mrbe_fb_load_rgba(MrbeImage *image, const char *path, int width, int height)
{
    FILE *file;
    size_t bytes;
    if (!image || !path || width <= 0 || height <= 0) return -1;
    memset(image, 0, sizeof(*image));
    bytes = (size_t)width * (size_t)height * 4u;
    file = fopen(path, "rb");
    if (!file) return -1;
    image->pixels = (uint8_t *)malloc(bytes);
    if (!image->pixels) {
        fclose(file);
        return -1;
    }
    if (fread(image->pixels, 1, bytes, file) != bytes) {
        free(image->pixels);
        memset(image, 0, sizeof(*image));
        fclose(file);
        return -1;
    }
    fclose(file);
    image->width = width;
    image->height = height;
    return 0;
}

void mrbe_fb_draw_rgba(MrbeFramebuffer *fb, const MrbeImage *image,
                       int x, int y, int width, int height, MrbeColor under)
{
    int dx, dy;
    if (!fb || !fb->memory || !image || !image->pixels ||
        image->width <= 0 || image->height <= 0 || width <= 0 || height <= 0)
        return;

    /* Fast nearest-neighbour blit. Opaque pixels bypass mrbe_pixel() so large
       cinematic artwork does not turn the framebuffer renderer into a CPU bottleneck. */
    for (dy = 0; dy < height; ++dy) {
        int dest_y = y + dy;
        int src_y = (int)((long long)dy * image->height / height);
        size_t row_offset;
        if (dest_y < 0 || dest_y >= (int)fb->variable.yres) continue;
        dest_y += (int)fb->variable.yoffset;
        row_offset = (size_t)dest_y * fb->fixed.line_length;
        for (dx = 0; dx < width; ++dx) {
            int dest_x = x + dx;
            int src_x, a;
            const uint8_t *src;
            MrbeColor out;
            uint32_t pixel;
            size_t offset;
            if (dest_x < 0 || dest_x >= (int)fb->variable.xres) continue;
            dest_x += (int)fb->variable.xoffset;
            src_x = (int)((long long)dx * image->width / width);
            src = image->pixels +
                  ((size_t)src_y * (size_t)image->width + (size_t)src_x) * 4u;
            a = src[3];
            if (a == 0) continue;
            if (a == 255) {
                out.r = src[0]; out.g = src[1]; out.b = src[2];
            } else {
                out.r = (uint8_t)(((unsigned int)src[0] * (unsigned int)a +
                                   (unsigned int)under.r * (unsigned int)(255-a)) / 255u);
                out.g = (uint8_t)(((unsigned int)src[1] * (unsigned int)a +
                                   (unsigned int)under.g * (unsigned int)(255-a)) / 255u);
                out.b = (uint8_t)(((unsigned int)src[2] * (unsigned int)a +
                                   (unsigned int)under.b * (unsigned int)(255-a)) / 255u);
            }
            pixel = mrbe_component(out.r, fb->variable.red) |
                    mrbe_component(out.g, fb->variable.green) |
                    mrbe_component(out.b, fb->variable.blue);
            offset = row_offset + (size_t)dest_x * (size_t)fb->bytes_per_pixel;
            if (offset + (size_t)fb->bytes_per_pixel > fb->memory_size) continue;
            memcpy(fb->memory + offset, &pixel, (size_t)fb->bytes_per_pixel);
        }
    }
}

void mrbe_fb_free_image(MrbeImage *image)
{
    if (!image) return;
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}
