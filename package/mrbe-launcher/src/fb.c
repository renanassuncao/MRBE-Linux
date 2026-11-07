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
    /* Common 8-bit channels need no rescaling (and no division per pixel). */
    if (field.length == 8) return (uint32_t)value << field.offset;
    if (field.length >= 32) maximum = 0xFFFFFFFFu;
    else maximum = (1u << field.length) - 1u;
    return (((uint32_t)value * maximum / 255u) & maximum) << field.offset;
}

static uint32_t mrbe_pack(const MrbeFramebuffer *fb, MrbeColor color)
{
    return mrbe_component(color.r, fb->variable.red) |
           mrbe_component(color.g, fb->variable.green) |
           mrbe_component(color.b, fb->variable.blue);
}

static void mrbe_pixel(MrbeFramebuffer *fb, int x, int y, MrbeColor color)
{
    size_t offset;
    uint32_t pixel;

    if (!fb || !fb->memory) return;
    if (x < 0 || y < 0 || x >= (int)fb->variable.xres || y >= (int)fb->variable.yres) return;

    offset = (size_t)y * fb->fixed.line_length + (size_t)x * (size_t)fb->bytes_per_pixel;
    if (offset + (size_t)fb->bytes_per_pixel > fb->memory_size) return;

    pixel = mrbe_pack(fb, color);
    memcpy(fb->memory + offset, &pixel, (size_t)fb->bytes_per_pixel);
}

int mrbe_fb_open(MrbeFramebuffer *fb, const char *device)
{
    size_t origin;

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

    fb->map_size = fb->fixed.smem_len;
    fb->map = (uint8_t *)mmap(NULL, fb->map_size, PROT_READ | PROT_WRITE,
                              MAP_SHARED, fb->fd, 0);
    if (fb->map == MAP_FAILED) {
        fb->map = NULL;
        perror("mmap framebuffer");
        mrbe_fb_close(fb);
        return -1;
    }

    origin = (size_t)fb->variable.yoffset * fb->fixed.line_length +
             (size_t)fb->variable.xoffset * (size_t)fb->bytes_per_pixel;
    if (origin >= fb->map_size) {
        fprintf(stderr, "Framebuffer offset outside the mapping\n");
        mrbe_fb_close(fb);
        return -1;
    }
    fb->screen = fb->map + origin;
    fb->memory_size = (size_t)fb->fixed.line_length * fb->variable.yres;
    if (fb->memory_size > fb->map_size - origin) fb->memory_size = fb->map_size - origin;

    /* Draw off-screen and copy whole frames, so a half-drawn screen never shows. */
    fb->memory = (uint8_t *)calloc(fb->memory_size, 1);
    if (!fb->memory) {
        fprintf(stderr, "No memory for a back buffer; drawing on the framebuffer\n");
        fb->memory = fb->screen;
    }

    fprintf(stderr, "MRBE framebuffer: %ux%u, %u bpp\n",
            fb->variable.xres, fb->variable.yres, fb->variable.bits_per_pixel);
    return 0;
}

void mrbe_fb_close(MrbeFramebuffer *fb)
{
    if (!fb) return;
    if (fb->map && fb->memory && fb->memory != fb->screen) free(fb->memory);
    if (fb->map) munmap(fb->map, fb->map_size);
    if (fb->fd >= 0) close(fb->fd);
    fb->memory = NULL;
    fb->map = NULL;
    fb->screen = NULL;
    fb->fd = -1;
}

void mrbe_fb_present(MrbeFramebuffer *fb)
{
    if (!fb || !fb->screen || !fb->memory || fb->memory == fb->screen) return;
    memcpy(fb->screen, fb->memory, fb->memory_size);
}

static void mrbe_fill_row(uint8_t *row, int count, uint32_t pixel, int bytes_per_pixel)
{
    int i;
    /* Constant-size copies compile to plain stores. */
    switch (bytes_per_pixel) {
        case 4: for (i = 0; i < count; ++i) memcpy(row + (size_t)i * 4u, &pixel, 4); break;
        case 2: for (i = 0; i < count; ++i) memcpy(row + (size_t)i * 2u, &pixel, 2); break;
        default: for (i = 0; i < count; ++i) memcpy(row + (size_t)i * 3u, &pixel, 3); break;
    }
}

void mrbe_fb_fill_rect(MrbeFramebuffer *fb, int x, int y, int width, int height, MrbeColor color)
{
    int right, bottom;
    size_t stride, left, span;
    uint8_t *first, *row;

    if (!fb || !fb->memory || width <= 0 || height <= 0) return;
    /* Clip once per rectangle instead of checking every pixel. */
    right = x + width;
    bottom = y + height;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > (int)fb->variable.xres) right = (int)fb->variable.xres;
    if (bottom > (int)fb->variable.yres) bottom = (int)fb->variable.yres;
    if (x >= right || y >= bottom) return;

    stride = fb->fixed.line_length;
    left = (size_t)x * (size_t)fb->bytes_per_pixel;
    span = (size_t)(right - x) * (size_t)fb->bytes_per_pixel;
    while (bottom > y && (size_t)(bottom - 1) * stride + left + span > fb->memory_size)
        --bottom;
    if (y >= bottom) return;

    /* Fill the first row, then copy it down. */
    first = fb->memory + (size_t)y * stride + left;
    mrbe_fill_row(first, right - x, mrbe_pack(fb, color), fb->bytes_per_pixel);
    for (row = first + stride, ++y; y < bottom; ++y, row += stride)
        memcpy(row, first, span);
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
    if (!fb || !image || !image->pixels || image->width <= 0 || image->height <= 0 ||
        width <= 0 || height <= 0) return;
    for (dy = 0; dy < height; ++dy) {
        int sy = (int)((long long)dy * image->height / height);
        for (dx = 0; dx < width; ++dx) {
            const uint8_t *src;
            unsigned int a;
            MrbeColor out;
            int sx = (int)((long long)dx * image->width / width);
            src = image->pixels + ((size_t)sy * (size_t)image->width + (size_t)sx) * 4u;
            a = src[3];
            if (a == 0) continue;
            out.r = (uint8_t)(((unsigned int)src[0] * a + (unsigned int)under.r * (255u-a)) / 255u);
            out.g = (uint8_t)(((unsigned int)src[1] * a + (unsigned int)under.g * (255u-a)) / 255u);
            out.b = (uint8_t)(((unsigned int)src[2] * a + (unsigned int)under.b * (255u-a)) / 255u);
            mrbe_pixel(fb, x + dx, y + dy, out);
        }
    }
}

void mrbe_fb_free_image(MrbeImage *image)
{
    if (!image) return;
    free(image->pixels);
    memset(image, 0, sizeof(*image));
}
