"""Framebuffer clipping, frame presentation and input hangup, checked on the host."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT/'package/mrbe-launcher/src'
FB_HARNESS = r'''
#include "fb.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define W 8
#define H 4
#define STRIDE (W*4+8)
static uint8_t back[STRIDE*H], front[STRIDE*H];
static uint32_t at(int x, int y) { uint32_t p; memcpy(&p, back+y*STRIDE+x*4, 4); return p; }
int main(void) {
    MrbeFramebuffer fb;
    int x, y;
    memset(&fb, 0, sizeof(fb));
    memset(back, 0xA5, sizeof(back));
    fb.memory = back; fb.memory_size = sizeof(back);
    fb.bytes_per_pixel = 4; fb.fixed.line_length = STRIDE;
    fb.variable.xres = W; fb.variable.yres = H; fb.variable.bits_per_pixel = 32;
    fb.variable.red.offset = 16; fb.variable.red.length = 8;
    fb.variable.green.offset = 8; fb.variable.green.length = 8;
    fb.variable.blue.offset = 0; fb.variable.blue.length = 8;
    /* Partly off the top-left corner and off the bottom-right corner. */
    mrbe_fb_fill_rect(&fb, -2, -1, 5, 3, (MrbeColor){255, 0, 0});
    mrbe_fb_fill_rect(&fb, 6, 3, 10, 10, (MrbeColor){0, 0, 255});
    mrbe_fb_fill_rect(&fb, 100, 100, 5, 5, (MrbeColor){0, 255, 0});
    for (y = 0; y < H; ++y) for (x = 0; x < W; ++x) {
        uint32_t want = 0xA5A5A5A5u;
        if (x < 3 && y < 2) want = 0x00FF0000u;
        if (x >= 6 && y == 3) want = 0x000000FFu;
        assert(at(x, y) == want);
    }
    for (y = 0; y < H; ++y)  /* Row padding is never written. */
        for (x = W*4; x < STRIDE; ++x) assert(back[y*STRIDE+x] == 0xA5);
    /* RGB565 packing. */
    {
        uint16_t p565[4] = {0};
        MrbeFramebuffer f16 = fb;
        f16.memory = (uint8_t *)p565; f16.memory_size = sizeof(p565);
        f16.bytes_per_pixel = 2; f16.fixed.line_length = sizeof(p565);
        f16.variable.xres = 4; f16.variable.yres = 1; f16.variable.bits_per_pixel = 16;
        f16.variable.red.offset = 11; f16.variable.red.length = 5;
        f16.variable.green.offset = 5; f16.variable.green.length = 6;
        f16.variable.blue.offset = 0; f16.variable.blue.length = 5;
        mrbe_fb_fill_rect(&f16, 1, 0, 2, 1, (MrbeColor){255, 0, 0});
        assert(p565[0] == 0 && p565[1] == 0xF800 && p565[2] == 0xF800 && p565[3] == 0);
    }
    /* Present copies the whole frame; with no back buffer it is a no-op. */
    memset(front, 0, sizeof(front));
    fb.map = front; fb.map_size = sizeof(front); fb.screen = front;
    mrbe_fb_present(&fb);
    assert(memcmp(front, back, sizeof(back)) == 0);
    fb.memory = front;
    mrbe_fb_present(&fb);
    return 0;
}
'''
INPUT_HARNESS = r'''
#include "input.h"
#include <assert.h>
#include <unistd.h>
int main(void) {
    MrbeInput in;
    int pipefd[2];
    assert(pipe(pipefd) == 0);
    in.fds[0] = pipefd[0];
    in.count = 1;
    close(pipefd[1]);  /* Hangup, like an unplugged evdev device. */
    assert(mrbe_input_next(&in, -1) == MRBE_KEY_NONE);
    assert(in.count == 0);  /* Dropped, so the launcher rescans instead of spinning. */
    assert(mrbe_input_next(&in, -1) == MRBE_KEY_NONE);
    return 0;
}
'''

class TestFramebuffer(unittest.TestCase):
    def _run(self, harness, *sources):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            (p/'harness.c').write_text(harness)
            exe = p/'harness'
            proc = subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                  '-I',str(SRC),str(p/'harness.c'),*[str(SRC/s) for s in sources],
                  '-o',str(exe)],capture_output=True,text=True)
            self.assertEqual(proc.returncode,0,proc.stderr)
            run = subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
            self.assertEqual(run.returncode,0,run.stderr)

    def test_fill_clipping_and_present(self):
        self._run(FB_HARNESS,'fb.c','font.c')

    def test_input_hangup_releases_devices(self):
        self._run(INPUT_HARNESS,'input.c')
