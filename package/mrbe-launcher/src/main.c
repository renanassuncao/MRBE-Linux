#include "fb.h"
#include "input.h"
#include "ui.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void report_console_marker(const char *marker)
{
    int fd;
    size_t length;
    if (!marker) return;
    fd = open("/dev/console", O_WRONLY);
    if (fd < 0) return;
    length = strlen(marker);
    (void)write(fd, marker, length);
    (void)write(fd, "\n", 1);
    close(fd);
}

int main(int argc, char **argv)
{
    MrbeFramebuffer fb;
    MrbeInput input;
    MrbeUIState ui;
    MrbeInputEvent event;
    float pointer_scale;
    int key;
    int input_ready_reported = 0;

    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        puts("MRBE TV OS Launcher 0.7.0");
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: mrbe-launcher [--version|--help]");
        puts("Arrows/Enter/Esc: navigate; H: home, A: apps, S: settings, Q: search, M: quick panel, F: favorite; mouse click selects.");
        return 0;
    }
    if (mrbe_fb_open(&fb, "/dev/fb0") != 0) {
        fprintf(stderr, "MRBE: /dev/fb0 indisponivel. Verifique virtio-gpu e fbdev.\n");
        return 1;
    }
    memset(&input, 0, sizeof(input));
    mrbe_ui_init(&ui);
    pointer_scale = (float)fb.variable.xres / 1280.0f;
    if ((float)fb.variable.yres / 720.0f < pointer_scale)
        pointer_scale = (float)fb.variable.yres / 720.0f;
    if (pointer_scale <= 0.0f) pointer_scale = 1.0f;
    mrbe_input_open(&input);
    if (mrbe_ui_home_assets_ready())
        report_console_marker("MRBE_HOME_ASSETS_READY");
    else
        fprintf(stderr, "[MRBE] Home assets incompletos; modo de fallback ativo\n");
    mrbe_ui_draw(&fb, &ui);
    report_console_marker("MRBE_UI_READY");
    if (input.count > 0) {
        report_console_marker("MRBE_INPUT_READY");
        input_ready_reported = 1;
    }
    fprintf(stderr, "[MRBE] primeira tela renderizada; launcher interativo pronto\n");
    fprintf(stderr, "[MRBE] mouse: mover para exibir ponteiro; clique esquerdo seleciona; direito volta; roda navega\n");
    for (;;) {
        if (input.count == 0) {
            sleep(1);
            mrbe_input_open(&input);
            if (input.count > 0 && !input_ready_reported) {
                report_console_marker("MRBE_INPUT_READY");
                input_ready_reported = 1;
            }
        }
        if (!mrbe_input_next_event(&input, &event, 150)) continue;
        key = event.key;
        if (event.mouse_moved || event.mouse_clicked) {
            ui.pointer_x += (int)((float)event.mouse_dx / pointer_scale);
            ui.pointer_y += (int)((float)event.mouse_dy / pointer_scale);
            if (ui.pointer_x < 0) ui.pointer_x = 0;
            if (ui.pointer_x > 1279) ui.pointer_x = 1279;
            if (ui.pointer_y < 0) ui.pointer_y = 0;
            if (ui.pointer_y > 719) ui.pointer_y = 719;
            mrbe_ui_handle_pointer(&ui, ui.pointer_x, ui.pointer_y, event.mouse_clicked);
        }
        if (key != MRBE_KEY_NONE)
            mrbe_ui_handle_key(&ui, key);
        if (event.mouse_moved || event.mouse_clicked || key != MRBE_KEY_NONE)
            mrbe_ui_draw(&fb, &ui);
    }
    /* Normally the launcher is stopped by init SIGTERM. */
    mrbe_input_close(&input);
    mrbe_fb_close(&fb);
    return 0;
}
