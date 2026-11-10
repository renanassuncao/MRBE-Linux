/* Host-only preview renderer for every MRBE TV OS 0.7.0 screen. */
#include "fb.h"
#include "ui.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void choose_screen(MrbeUIState *ui, const char *name)
{
    if (!name || strcmp(name, "home") == 0) return;
    if (strcmp(name, "apps") == 0) { ui->screen = MRBE_SCREEN_APPS; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 2; return; }
    if (strcmp(name, "store") == 0) { ui->screen = MRBE_SCREEN_STORE; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 3; return; }
    if (strcmp(name, "library") == 0) { ui->screen = MRBE_SCREEN_LIBRARY; ui->focus = MRBE_FOCUS_CATEGORIES; ui->nav_index = 5; return; }
    if (strcmp(name, "settings") == 0) { ui->screen = MRBE_SCREEN_SETTINGS; ui->focus = MRBE_FOCUS_SETTINGS; ui->nav_index = 6; return; }
    if (strcmp(name, "search") == 0) { ui->screen = MRBE_SCREEN_SEARCH; ui->focus = MRBE_FOCUS_SEARCH; return; }
    if (strcmp(name, "details") == 0) {
        ui->screen = MRBE_SCREEN_DETAILS; ui->focus = MRBE_FOCUS_DETAIL_ACTIONS;
        ui->nav_index = 2; ui->detail_app_index = 0; return;
    }
    if (strcmp(name, "films") == 0) { ui->screen = MRBE_SCREEN_FILMS; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 1; return; }
    if (strcmp(name, "series") == 0) { ui->screen = MRBE_SCREEN_SERIES; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 1; return; }
    if (strcmp(name, "sports") == 0) { ui->screen = MRBE_SCREEN_SPORTS; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 1; return; }
    if (strcmp(name, "kids") == 0) { ui->screen = MRBE_SCREEN_KIDS; ui->focus = MRBE_FOCUS_GRID; ui->nav_index = 1; return; }
    fprintf(stderr, "Unknown screen: %s\n", name);
    exit(2);
}

int main(int argc, char **argv)
{
    const int w = 1280, h = 720;
    const size_t size = (size_t)w * (size_t)h * 4u;
    MrbeFramebuffer fb;
    MrbeUIState ui;
    FILE *out;
    int x, y;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: mrbe-preview output.ppm [home|apps|store|library|settings|search|details|films|series|sports|kids]\n");
        return 2;
    }
    memset(&fb, 0, sizeof(fb));
    fb.memory = calloc(size, 1);
    if (!fb.memory) return 2;
    fb.memory_size = size;
    fb.bytes_per_pixel = 4;
    fb.fixed.line_length = w * 4;
    fb.variable.xres = w;
    fb.variable.yres = h;
    fb.variable.bits_per_pixel = 32;
    fb.variable.red.offset = 16; fb.variable.red.length = 8;
    fb.variable.green.offset = 8; fb.variable.green.length = 8;
    fb.variable.blue.offset = 0; fb.variable.blue.length = 8;

    mrbe_ui_init(&ui);
    if (argc == 3) choose_screen(&ui, argv[2]);
    mrbe_ui_draw(&fb, &ui);

    out = fopen(argv[1], "wb");
    if (!out) { perror("fopen"); free(fb.memory); return 2; }
    fprintf(out, "P6\n%d %d\n255\n", w, h);
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            size_t i = ((size_t)y * (size_t)w + (size_t)x) * 4u;
            uint8_t pixel[3] = {fb.memory[i + 2], fb.memory[i + 1], fb.memory[i]};
            if (fwrite(pixel, 1, 3, out) != 3) {
                perror("write"); fclose(out); free(fb.memory); return 2;
            }
        }
    }
    fclose(out);
    free(fb.memory);
    return 0;
}
