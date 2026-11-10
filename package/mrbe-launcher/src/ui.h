#ifndef MRBE_UI_H
#define MRBE_UI_H

#include "fb.h"
#include "input.h"

#define MRBE_APP_COUNT 12

typedef enum {
    MRBE_SCREEN_HOME = 0,
    MRBE_SCREEN_APPS,
    MRBE_SCREEN_STORE,
    MRBE_SCREEN_FILMS,
    MRBE_SCREEN_SERIES,
    MRBE_SCREEN_SPORTS,
    MRBE_SCREEN_KIDS,
    MRBE_SCREEN_LIBRARY,
    MRBE_SCREEN_SETTINGS,
    MRBE_SCREEN_SEARCH,
    MRBE_SCREEN_DETAILS
} MrbeScreen;

typedef enum {
    MRBE_FOCUS_NAV = 0,
    MRBE_FOCUS_HERO,
    MRBE_FOCUS_HOME_APPS,
    MRBE_FOCUS_HOME_CONTENT,
    MRBE_FOCUS_CATEGORIES,
    MRBE_FOCUS_GRID,
    MRBE_FOCUS_SETTINGS,
    MRBE_FOCUS_DETAIL_ACTIONS,
    MRBE_FOCUS_SEARCH
} MrbeFocus;

typedef struct {
    MrbeScreen screen;
    MrbeScreen return_screen;
    MrbeFocus focus;
    MrbeFocus return_focus;
    int nav_index;
    int hero_index;
    int home_app_index;
    int home_content_index;
    int grid_index;
    int category_index;
    int settings_index;
    int detail_app_index;
    int detail_action;
    int search_index;
    int quick_panel;
    int quick_index;
    int pointer_x;
    int pointer_y;
    int pointer_visible;
    unsigned int favorite_mask;
    unsigned int viewed_mask;
    int show_notice;
    char notice[128];
} MrbeUIState;

void mrbe_ui_init(MrbeUIState *state);
void mrbe_ui_handle_key(MrbeUIState *state, int key);
void mrbe_ui_handle_pointer(MrbeUIState *state, int x, int y, int clicked);
void mrbe_ui_draw(MrbeFramebuffer *fb, const MrbeUIState *state);
int mrbe_ui_home_assets_ready(void);

#endif
