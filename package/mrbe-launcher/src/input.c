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
    /* Close descriptors when rescanning to avoid leaks on reconnect. */
    mrbe_input_close(input);
    for (i = 0; i < 32; ++i) input->fds[i] = -1;

    for (i = 0; i < 32; ++i) {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) input->fds[input->count++] = fd;
    }
    if (input->count == 0)
        fprintf(stderr, "MRBE: no evdev input devices found yet\n");
    else
        fprintf(stderr, "MRBE: opened %d evdev input device(s)\n", input->count);
    return input->count;
}

void mrbe_input_close(MrbeInput *input)
{
    int i;
    if (!input) return;
    for (i = 0; i < input->count; ++i)
        if (input->fds[i] >= 0) close(input->fds[i]);
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
        case KEY_ESC:
        case KEY_BACKSPACE:
        case KEY_BACK: return MRBE_KEY_BACK;
        default: return MRBE_KEY_NONE;
    }
}

int mrbe_input_next(MrbeInput *input, int timeout_ms)
{
    struct pollfd descriptors[32];
    int i, result;
    if (!input || input->count == 0) {
        struct timespec delay;
        if (timeout_ms <= 0) return MRBE_KEY_NONE;
        delay.tv_sec = timeout_ms / 1000;
        delay.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
        nanosleep(&delay, NULL);
        return MRBE_KEY_NONE;
    }

    for (i = 0; i < input->count; ++i) {
        descriptors[i].fd = input->fds[i];
        descriptors[i].events = POLLIN;
        descriptors[i].revents = 0;
    }

    result = poll(descriptors, (nfds_t)input->count, timeout_ms);
    if (result <= 0) return MRBE_KEY_NONE;

    for (i = 0; i < input->count; ++i) {
        if (descriptors[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            /* Unplugged device: poll reports it forever and the loop would spin
             * at 100% CPU. Drop all descriptors; the caller rescans /dev/input. */
            mrbe_input_close(input);
            return MRBE_KEY_NONE;
        }
        if (descriptors[i].revents & POLLIN) {
            struct input_event event;
            ssize_t bytes = read(descriptors[i].fd, &event, sizeof(event));
            if (bytes == (ssize_t)sizeof(event) &&
                event.type == EV_KEY && event.value == 1)
                return mrbe_map_key(event.code);
        }
    }
    return MRBE_KEY_NONE;
}
