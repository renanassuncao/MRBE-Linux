#define _POSIX_C_SOURCE 200809L
#include "fb.h"
#include "input.h"
#include "ui.h"

#include <fcntl.h>
#include <linux/kd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int console = -1;

static void console_text(void)
{
    if (console >= 0) ioctl(console, KDSETMODE, KD_TEXT);
}

static void on_signal(int sig)
{
    console_text();
    signal(sig, SIG_DFL);
    raise(sig);
}

/* In text mode fbcon keeps blinking its cursor over the framebuffer and draws
 * any console output on top of the launcher. Graphics mode stops both; signal
 * handlers put the VT back in text mode when init stops the launcher. */
static void console_graphics(void)
{
    static const int signals[] = { SIGTERM, SIGINT, SIGHUP, SIGSEGV, SIGABRT };
    struct sigaction action;
    size_t i;
    console = open("/dev/tty0", O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (console < 0) return;
    if (ioctl(console, KDSETMODE, KD_GRAPHICS) != 0) {
        close(console);
        console = -1;
        return;
    }
    memset(&action, 0, sizeof(action));
    action.sa_handler = on_signal;
    for (i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i)
        sigaction(signals[i], &action, NULL);
}

/* The kernel log stamps the first frame on the same clock as boot messages,
 * so make profile can tell how long the screen took after power-on. */
static void log_first_frame(const MrbeFramebuffer *fb)
{
    char line[64];
    int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    int n;
    if (fd < 0) return;
    n = snprintf(line, sizeof(line), "<6>mrbe-launcher: primeira tela %ux%u\n",
                 fb->variable.xres, fb->variable.yres);
    if (n > 0 && write(fd, line, (size_t)n) < 0)
        perror("MRBE: /dev/kmsg");
    close(fd);
}

int main(int argc, char **argv)
{
    MrbeFramebuffer fb;
    MrbeInput input;
    MrbeUIState ui, previous;
    int key;

    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        puts("MRBE Linux TV Launcher 0.4.0");
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        puts("Usage: mrbe-launcher [--version|--help]");
        puts("Arrows/Enter/Esc: navigate; H: home, A: apps, S: settings.");
        return 0;
    }
    if (mrbe_fb_open(&fb, "/dev/fb0") != 0) {
        fprintf(stderr, "MRBE: /dev/fb0 indisponivel. Verifique virtio-gpu e fbdev.\n");
        return 1;
    }
    console_graphics();
    memset(&input, 0, sizeof(input));
    mrbe_ui_init(&ui);
    mrbe_input_open(&input);
    mrbe_ui_draw(&fb, &ui);
    mrbe_fb_present(&fb);
    log_first_frame(&fb);
    for (;;) {
        if (input.count == 0) {
            sleep(1);
            mrbe_input_open(&input);
        }
        /* Block until input arrives: no periodic wakeups while idle. */
        key = mrbe_input_next(&input, -1);
        if (key == MRBE_KEY_NONE) continue;
        previous = ui;
        mrbe_ui_handle_key(&ui, key);
        /* Keys that change nothing (LEFT on the first item, etc.) skip the redraw. */
        if (memcmp(&previous, &ui, sizeof(ui)) == 0) continue;
        mrbe_ui_draw(&fb, &ui);
        mrbe_fb_present(&fb);
    }
    /* Normally the launcher is stopped by init SIGTERM. */
    mrbe_input_close(&input);
    mrbe_fb_close(&fb);
    console_text();
    return 0;
}
