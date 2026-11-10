"""State-machine and pointer tests for MRBE TV OS 0.7.0; no QEMU required."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "package/mrbe-launcher/src"
HARNESS = r"""
#include "ui.h"
#include <assert.h>
#include <string.h>

int main(void) {
    MrbeUIState s;
    mrbe_ui_init(&s);
    assert(s.screen == MRBE_SCREEN_HOME);
    assert(s.focus == MRBE_FOCUS_NAV);
    assert(s.nav_index == 0);

    /* Home hero actions: first button opens Guide; second opens Store. */
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.focus == MRBE_FOCUS_HERO);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_STORE);
    assert(s.nav_index == 3);

    /* Store category -> grid -> app details -> favorite. */
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    assert(s.focus == MRBE_FOCUS_GRID);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_DETAILS);
    assert(s.detail_app_index == 0);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.detail_action == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert((s.favorite_mask & 1u) != 0);
    mrbe_ui_handle_key(&s, MRBE_KEY_BACK);
    assert(s.screen == MRBE_SCREEN_STORE);
    assert(s.focus == MRBE_FOCUS_GRID);

    /* Apps shortcut and grid selection. */
    mrbe_ui_handle_key(&s, MRBE_KEY_APPS);
    assert(s.screen == MRBE_SCREEN_APPS);
    assert(s.nav_index == 2);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    assert(s.focus == MRBE_FOCUS_GRID);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.grid_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_FAVORITE);
    assert((s.favorite_mask & (1u << 1)) != 0);
    mrbe_ui_handle_key(&s, MRBE_KEY_UP);
    assert(s.focus == MRBE_FOCUS_CATEGORIES);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.category_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    assert(s.focus == MRBE_FOCUS_GRID);
    assert(s.grid_index == 0);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.grid_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_DETAILS);
    assert(s.detail_app_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_BACK);
    assert(s.screen == MRBE_SCREEN_APPS);

    /* Library shows favorites and allows opening details. */
    mrbe_ui_handle_key(&s, MRBE_KEY_BACK);
    assert(s.screen == MRBE_SCREEN_HOME);
    for (int i = 0; i < 5; ++i) mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_LIBRARY);

    /* Settings and search shortcuts. */
    mrbe_ui_handle_key(&s, MRBE_KEY_SETTINGS);
    assert(s.screen == MRBE_SCREEN_SETTINGS);
    assert(s.nav_index == 6);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    assert(s.settings_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_SEARCH);
    assert(s.screen == MRBE_SCREEN_SEARCH);
    assert(s.nav_index == 4);

    /* Quick panel opens, navigates and closes. */
    mrbe_ui_handle_key(&s, MRBE_KEY_MENU);
    assert(s.quick_panel == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    assert(s.quick_index == 1);
    mrbe_ui_handle_key(&s, MRBE_KEY_BACK);
    assert(s.quick_panel == 0);

    /* Film pages open with visible focus and cannot escape the eight cards. */
    mrbe_ui_handle_key(&s, MRBE_KEY_HOME);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_FILMS);
    assert(s.focus == MRBE_FOCUS_GRID);
    s.grid_index = 7;
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    assert(s.grid_index == 7);

    /* Details opened from Home restores focus to the Home app row. */
    mrbe_ui_handle_key(&s, MRBE_KEY_HOME);
    mrbe_ui_handle_key(&s, MRBE_KEY_RIGHT);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    mrbe_ui_handle_key(&s, MRBE_KEY_DOWN);
    s.home_app_index = 3;
    mrbe_ui_handle_key(&s, MRBE_KEY_ENTER);
    assert(s.screen == MRBE_SCREEN_DETAILS);
    mrbe_ui_handle_key(&s, MRBE_KEY_BACK);
    assert(s.screen == MRBE_SCREEN_HOME);
    assert(s.focus == MRBE_FOCUS_HOME_APPS);

    /* Mouse hit-testing: sidebar opens Store; card opens details; click favorite. */
    s.favorite_mask &= ~1u;
    mrbe_ui_handle_pointer(&s, 80, 362, 0);
    assert(s.screen == MRBE_SCREEN_HOME); /* Hover must not navigate. */
    mrbe_ui_handle_pointer(&s, 80, 362, 1);
    assert(s.screen == MRBE_SCREEN_STORE);
    assert(s.nav_index == 3);
    mrbe_ui_handle_pointer(&s, 300, 260, 1);
    assert(s.screen == MRBE_SCREEN_DETAILS);
    assert(s.detail_app_index == 0);
    mrbe_ui_handle_pointer(&s, 500, 544, 1);
    assert((s.favorite_mask & 1u) != 0);
    mrbe_ui_handle_pointer(&s, 680, 544, 1);
    assert(s.screen == MRBE_SCREEN_STORE);
    assert(s.pointer_visible == 1);

    /* H always returns to Home. */
    mrbe_ui_handle_key(&s, MRBE_KEY_HOME);
    assert(s.screen == MRBE_SCREEN_HOME);
    assert(strlen(s.notice) < sizeof(s.notice));
    return 0;
}
"""

class TestUI(unittest.TestCase):
    def test_keyboard_navigation_and_screens(self):
        with tempfile.TemporaryDirectory() as temp:
            tmp = Path(temp)
            (tmp / "test-ui.c").write_text(HARNESS)
            exe = tmp / "test-ui"
            proc = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-I", str(SRC), str(tmp / "test-ui.c"), str(SRC / "ui.c"),
                 str(SRC / "fb.c"), str(SRC / "font.c"), "-o", str(exe)],
                capture_output=True, text=True
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)


    def test_evdev_input_compiles(self):
        with tempfile.TemporaryDirectory() as temp:
            obj = Path(temp) / "input.o"
            proc = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-c",
                 str(SRC / "input.c"), "-o", str(obj)],
                capture_output=True, text=True
            )
            self.assertEqual(proc.returncode, 0, proc.stderr)
            self.assertTrue(obj.is_file())

    def test_launcher_source_version(self):
        self.assertIn("0.7.0", (SRC / "main.c").read_text())
        self.assertIn("mrbe_ui_home_assets_ready", (SRC / "ui.h").read_text())
        self.assertIn("MRBE_SCREEN_STORE", (SRC / "ui.h").read_text())
        self.assertIn("MRBE_KEY_FAVORITE", (SRC / "input.h").read_text())
        self.assertIn("mrbe_ui_handle_pointer", (SRC / "ui.h").read_text())
        self.assertIn("mrbe_input_next_event", (SRC / "input.h").read_text())
        self.assertIn("MRBE TV OS 0.7.0", (SRC / "ui.c").read_text())

if __name__ == "__main__":
    unittest.main()
