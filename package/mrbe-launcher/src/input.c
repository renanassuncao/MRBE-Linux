#define _POSIX_C_SOURCE 200809L
#include "input.h"

#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int mrbe_input_open(MrbeInput *input)
{
    int i;
    char path[64];
    if (!input) return 0;

    mrbe_input_close(input);
    for (i = 0; i < 32; ++i) input->fds[i] = -1;

    for (i = 0; i < 32; ++i) {
        int fd;
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) input->fds[input->count++] = fd;
    }

    if (input->count == 0)
        fprintf(stderr, "MRBE: no evdev input devices found yet\n");
    else
        fprintf(stderr, "MRBE: opened %d evdev input device(s); keyboard/mouse events enabled\n",
                input->count);
    return input->count;
}

void mrbe_input_close(MrbeInput *input)
{
    int i;
    if (!input) return;
    for (i = 0; i < input->count; ++i) {
        if (input->fds[i] >= 0) close(input->fds[i]);
        input->fds[i] = -1;
    }
    input->count = 0;
}

static int mrbe_map_key(unsigned int code)
{
    switch (code) {
        case KEY_LEFT: return MRBE_KEY_LEFT;
        case KEY_RIGHT: return MRBE_KEY_RIGHT;
        case KEY_UP: return MRBE_KEY_UP;
        case KEY_DOWN: return MRBE_KEY_DOWN;
        case KEY_ENTER:
        case KEY_KPENTER:
        case KEY_OK: return MRBE_KEY_ENTER;
        case KEY_H: return MRBE_KEY_HOME;
        case KEY_A: return MRBE_KEY_APPS;
        case KEY_S: return MRBE_KEY_SETTINGS;
        case KEY_Q: return MRBE_KEY_SEARCH;
        case KEY_M: return MRBE_KEY_MENU;
        case KEY_F: return MRBE_KEY_FAVORITE;
        case KEY_ESC:
        case KEY_BACKSPACE:
        case KEY_BACK: return MRBE_KEY_BACK;
        default: return MRBE_KEY_NONE;
    }
}

int mrbe_input_next_event(MrbeInput *input, MrbeInputEvent *event, int timeout_ms)
{
    struct pollfd descriptors[32];
    int i, result, any = 0;

    if (!event) return 0;
    memset(event, 0, sizeof(*event));

    if (!input || input->count == 0) {
        struct timespec delay;
        delay.tv_sec = timeout_ms / 1000;
        delay.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
        nanosleep(&delay, NULL);
        return 0;
    }

    for (i = 0; i < input->count; ++i) {
        descriptors[i].fd = input->fds[i];
        descriptors[i].events = POLLIN;
        descriptors[i].revents = 0;
    }

    result = poll(descriptors, (nfds_t)input->count, timeout_ms);
    if (result <= 0) return 0;

    for (i = 0; i < input->count; ++i) {
        if (!(descriptors[i].revents & POLLIN)) continue;

        /* Drain this evdev queue so X/Y and button events are processed together. */
        for (;;) {
            struct input_event ev;
            ssize_t bytes = read(descriptors[i].fd, &ev, sizeof(ev));
            if (bytes != (ssize_t)sizeof(ev)) break;
            any = 1;

            if (ev.type == EV_REL) {
                if (ev.code == REL_X) event->mouse_dx += ev.value;
                else if (ev.code == REL_Y) event->mouse_dy += ev.value;
                else if (ev.code == REL_WHEEL) {
                    if (ev.value > 0) event->key = MRBE_KEY_UP;
                    else if (ev.value < 0) event->key = MRBE_KEY_DOWN;
                } else if (ev.code == REL_HWHEEL) {
                    if (ev.value > 0) event->key = MRBE_KEY_RIGHT;
                    else if (ev.value < 0) event->key = MRBE_KEY_LEFT;
                }
            } else if (ev.type == EV_KEY) {
                if (ev.code == BTN_LEFT && ev.value == 1) {
                    event->mouse_clicked = 1;
                } else if (ev.code == BTN_RIGHT && ev.value == 1) {
                    event->key = MRBE_KEY_BACK;
                } else if (ev.code == BTN_MIDDLE && ev.value == 1) {
                    event->key = MRBE_KEY_MENU;
                } else if (ev.value == 1 && event->key == MRBE_KEY_NONE) {
                    event->key = mrbe_map_key(ev.code);
                }
            }
        }
    }

    if (event->mouse_dx != 0 || event->mouse_dy != 0)
        event->mouse_moved = 1;
    return any;
}

int mrbe_input_next(MrbeInput *input, int timeout_ms)
{
    MrbeInputEvent event;
    if (!mrbe_input_next_event(input, &event, timeout_ms))
        return MRBE_KEY_NONE;
    return event.key;
}
