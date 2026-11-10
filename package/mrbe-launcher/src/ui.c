/*
 * MRBE TV OS 0.7.0
 * Premium TV launcher rendered directly to Linux framebuffer.
 * The catalog and streaming tiles are demonstrative; no proprietary apps/DRM
 * are installed or launched by this Linux/QEMU build.
 */
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define CANVAS_W 1280
#define CANVAS_H 720
#define SIDEBAR_W 160
#define MAIN_X 180

#ifndef MRBE_UI_ASSET_DIR
#define MRBE_UI_ASSET_DIR "/usr/share/mrbe/ui"
#endif

typedef struct {
    MrbeFramebuffer *fb;
    int w, h;
    float scale;
    int offset_x, offset_y;
} View;

static const MrbeColor BG       = {  7,  11,  10};
static const MrbeColor SIDEBAR  = { 10,  19,  17};
static const MrbeColor SURFACE  = { 20,  31,  28};
static const MrbeColor SURFACE2 = { 27,  43,  37};
static const MrbeColor WHITE    = {244, 247, 245};
static const MrbeColor SUB      = {163, 178, 170};
static const MrbeColor GOLD     = {205, 169,  91};
static const MrbeColor GREEN    = { 37, 165, 112};
static MrbeImage brand_lockup;
static MrbeImage home_hero;
static MrbeImage home_cards[5];
static MrbeImage app_tiles[9];
static MrbeImage nav_icons[7];
static MrbeImage nav_labels[7];
static int ui_assets_checked;

static const char *NAV_LABELS[] = {
    "HOME", "GUIA", "APPS", "LOJA", "PESQUISAR", "BIBLIOTECA", "CONFIG"
};
static const char *NAV_ASSETS[] = {
    "home", "guide", "apps", "store", "search", "library", "settings"
};

static const char *APP_NAMES[MRBE_APP_COUNT] = {
    "NETFLIX", "DISNEY+", "PRIME VIDEO", "MAX",
    "YOUTUBE", "KODI", "VLC PLAYER", "ARQUIVOS",
    "NAVEGADOR", "MUSICA", "DIAGNOSTICO", "SOBRE MRBE"
};
static const char *APP_DESCRIPTIONS[MRBE_APP_COUNT] = {
    "FILMES E SERIES POR STREAMING",
    "FILMES, SERIES E PRODUCOES DISNEY",
    "FILMES E SERIES PRIME VIDEO",
    "FILMES, SERIES E CONTEUDO MAX",
    "VIDEOS E CANAIS",
    "CENTRAL DE MIDIA LOCAL",
    "REPRODUTOR MULTIMIDIA",
    "GERENCIADOR DE ARQUIVOS",
    "NAVEGACAO NA INTERNET",
    "REPRODUCAO DE AUDIO",
    "INFORMACOES E TESTES DO SISTEMA",
    "IDENTIDADE E VERSAO DO MRBE"
};
static const MrbeColor APP_COLORS[MRBE_APP_COUNT] = {
    {190,  32,  42}, { 26,  65, 139}, { 36, 113, 190}, { 82,  50, 159},
    {190,  36,  36}, { 36,  91,  76}, { 48,  91, 155}, { 78,  91,  86},
    { 40,  93, 117}, {116,  75, 144}, { 37, 121,  84}, {151, 117,  58}
};
static const char *STORE_CATEGORIES[] = {
    "DESTAQUES", "STREAMING", "MIDIA", "UTILITARIOS"
};
static const char *SETTINGS_NAMES[] = {
    "REDE E INTERNET", "TELA E IMAGEM", "SOM", "CONTROLE REMOTO",
    "APLICATIVOS", "ARMAZENAMENTO", "SISTEMA E ATUALIZACOES", "SOBRE O MRBE"
};
static const char *CONTENT_NAMES[] = {
    "ALEM DO HORIZONTE", "ROTAS DO PLANETA", "NOITE E CIDADE", "MUNDO SELVAGEM",
    "CINEMA MRBE", "DOCUMENTARIOS", "MUSICA AO VIVO", "PAISAGENS"
};

/* Keep the 16:9 design canvas proportional on 4:3/odd fallback framebuffers. */
static int sx(const View *v, int x)
{
    return v->offset_x + (int)((float)x * v->scale + 0.5f);
}
static int sy(const View *v, int y)
{
    return v->offset_y + (int)((float)y * v->scale + 0.5f);
}
static int maxi(int a, int b) { return a > b ? a : b; }
static int mini(int a, int b) { return a < b ? a : b; }

static void rect(View *v, int x, int y, int w, int h, MrbeColor c)
{
    int px, py, pw, ph;
    if (w < 1 || h < 1) return;
    px = sx(v, x); py = sy(v, y);
    pw = sx(v, x + w) - px; ph = sy(v, y + h) - py;
    mrbe_fb_fill_rect(v->fb, px, py, maxi(1, pw), maxi(1, ph), c);
}

static void roundrect(View *v, int x, int y, int w, int h, int r, MrbeColor c)
{
    int i, cut;
    if (w < 1 || h < 1) return;
    r = mini(r, mini(w / 2, h / 2));
    if (r < 1) { rect(v, x, y, w, h, c); return; }
    rect(v, x + r, y, w - 2 * r, h, c);
    rect(v, x, y + r, r, h - 2 * r, c);
    rect(v, x + w - r, y + r, r, h - 2 * r, c);
    for (i = 0; i < r; ++i) {
        cut = (r - i) * (r - i) / (r * 2);
        rect(v, x + cut, y + i, w - 2 * cut, 1, c);
        rect(v, x + cut, y + h - 1 - i, w - 2 * cut, 1, c);
    }
}

static void outline(View *v, int x, int y, int w, int h, MrbeColor c)
{
    rect(v, x, y, w, 3, c);
    rect(v, x, y + h - 3, w, 3, c);
    rect(v, x, y, 3, h, c);
    rect(v, x + w - 3, y, 3, h, c);
}

static void text(View *v, int x, int y, int size, MrbeColor c, const char *s)
{
    int scale = maxi(1, (int)(v->scale * (float)size + 0.5f));
    mrbe_fb_text(v->fb, sx(v, x), sy(v, y), scale, c, s);
}

static void center(View *v, int x, int y, int size, MrbeColor c, const char *s)
{
    int scale = maxi(1, (int)(v->scale * (float)size + 0.5f));
    mrbe_fb_text_center(v->fb, sx(v, x), sy(v, y), scale, c, s);
}

static void draw_image(View *v, const MrbeImage *image,
                       int x, int y, int w, int h, MrbeColor under)
{
    int px, py, pw, ph;
    if (!image || !image->pixels || w < 1 || h < 1) return;
    px = sx(v, x);
    py = sy(v, y);
    pw = sx(v, x + w) - px;
    ph = sy(v, y + h) - py;
    mrbe_fb_draw_rgba(v->fb, image, px, py, maxi(1, pw), maxi(1, ph), under);
}

static int load_ui_asset(MrbeImage *image, const char *name, int width, int height)
{
    char path[320];
    if (!image || !name) return -1;
    snprintf(path, sizeof(path), "%s/%s.rgba", MRBE_UI_ASSET_DIR, name);
    if (mrbe_fb_load_rgba(image, path, width, height) == 0) return 0;
    /* Host preview fallback; Buildroot uses /usr/share/mrbe/ui. */
    snprintf(path, sizeof(path), "package/mrbe-launcher/src/assets/%s.rgba", name);
    return mrbe_fb_load_rgba(image, path, width, height);
}

static void load_ui_assets(void)
{
    int i;
    char name[40];
    if (ui_assets_checked) return;
    ui_assets_checked = 1;
    if (load_ui_asset(&brand_lockup, "brand-lockup", 320, 230) != 0)
        fprintf(stderr, "[MRBE] brand-lockup ausente; usando marca vetorial\n");
    if (load_ui_asset(&home_hero, "home-hero", 1120, 340) != 0)
        fprintf(stderr, "[MRBE] home-hero ausente; usando arte vetorial\n");
    for (i = 0; i < 5; ++i) {
        snprintf(name, sizeof(name), "home-card-%d", i);
        if (load_ui_asset(&home_cards[i], name, 400, 250) != 0)
            fprintf(stderr, "[MRBE] asset %s ausente\n", name);
    }
    for (i = 0; i < 9; ++i) {
        snprintf(name, sizeof(name), "app-tile-%d", i);
        if (load_ui_asset(&app_tiles[i], name, 360, 220) != 0)
            fprintf(stderr, "[MRBE] asset %s ausente\n", name);
    }
    for (i = 0; i < 7; ++i) {
        snprintf(name, sizeof(name), "nav-%s", NAV_ASSETS[i]);
        if (load_ui_asset(&nav_icons[i], name, 128, 128) != 0)
            fprintf(stderr, "[MRBE] asset %s ausente\n", name);
        snprintf(name, sizeof(name), "nav-label-%d", i);
        if (load_ui_asset(&nav_labels[i], name, 160, 40) != 0)
            fprintf(stderr, "[MRBE] asset %s ausente\n", name);
    }
}


int mrbe_ui_home_assets_ready(void)
{
    int i;
    if (!home_hero.pixels || !brand_lockup.pixels) return 0;
    for (i = 0; i < 5; ++i) if (!home_cards[i].pixels) return 0;
    for (i = 0; i < 9; ++i) if (!app_tiles[i].pixels) return 0;
    for (i = 0; i < 7; ++i)
        if (!nav_icons[i].pixels || !nav_labels[i].pixels) return 0;
    return 1;
}

static void disk(View *v, int cx, int cy, int radius, MrbeColor c)
{
    int y, dx = radius;
    for (y = 0; y <= radius; ++y) {
        while (dx > 0 && dx * dx + y * y > radius * radius) --dx;
        rect(v, cx - dx, cy + y, 2 * dx + 1, 1, c);
        if (y) rect(v, cx - dx, cy - y, 2 * dx + 1, 1, c);
    }
}

static void background(View *v)
{
    int y;
    mrbe_fb_clear(v->fb, BG);
    for (y = 0; y < CANVAS_H; y += 24) {
        MrbeColor c = {
            (unsigned char)(5 + y / 260),
            (unsigned char)(13 + y / 190),
            (unsigned char)(10 + y / 220)
        };
        rect(v, SIDEBAR_W, y, CANVAS_W - SIDEBAR_W, 24, c);
    }
    rect(v, 0, 0, SIDEBAR_W, CANVAS_H, (MrbeColor){4, 18, 15});
    rect(v, SIDEBAR_W - 2, 0, 2, CANVAS_H, (MrbeColor){23, 64, 48});
}

static void brand(View *v)
{
    load_ui_assets();
    if (brand_lockup.pixels) {
        draw_image(v, &brand_lockup, 13, 13, 134, 100, SIDEBAR);
    } else {
        text(v, 24, 35, 3, WHITE, "MRBE");
        text(v, 40, 66, 1, GOLD, "TV OS");
    }
    rect(v, 16, 132, 128, 1, (MrbeColor){39, 72, 57});
}

static void nav_icon(View *v, int index, int x, int y, int active)
{
    if (index < 0 || index >= 7) return;
    if (nav_icons[index].pixels)
        draw_image(v, &nav_icons[index], x, y, 31, 31, SIDEBAR);
    else {
        MrbeColor fg = active ? GOLD : SUB;
        roundrect(v, x + 3, y + 3, 25, 25, 7, SURFACE2);
        center(v, x + 15, y + 11, 1, fg, index == 0 ? "H" :
               index == 1 ? "G" : index == 2 ? "A" :
               index == 3 ? "L" : index == 4 ? "P" :
               index == 5 ? "B" : "C");
    }
}

static void sidebar(View *v, const MrbeUIState *s)
{
    int i, y;
    brand(v);
    for (i = 0; i < 7; ++i) {
        y = 166 + i * 62;
        if (s->nav_index == i) {
            roundrect(v, 6, y, 148, 47, 14, (MrbeColor){13, 67, 47});
            outline(v, 6, y, 148, 47, (MrbeColor){28, 180, 117});
        }
        nav_icon(v, i, 17, y + 8, s->nav_index == i);
        if (nav_labels[i].pixels)
            draw_image(v, &nav_labels[i], 56, y + 9, 95, 29, SIDEBAR);
        else
            text(v, 57, y + 18, 1, s->nav_index == i ? WHITE : SUB, NAV_LABELS[i]);
    }
    text(v, 13, 686, 1, (MrbeColor){91, 126, 108}, "MRBE TV OS 0.7.0");
}

static void draw_search_icon(View *v, int x, int y, MrbeColor c)
{
    disk(v, x, y, 8, c);
    disk(v, x, y, 5, BG);
    rect(v, x + 6, y + 6, 10, 3, c);
}

static void top_status(View *v)
{
    char clock_text[8] = "--:--";
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (tm_now) strftime(clock_text, sizeof(clock_text), "%H:%M", tm_now);
    roundrect(v, 1030, 16, 230, 49, 16, (MrbeColor){4, 21, 18});
    outline(v, 1030, 16, 230, 49, (MrbeColor){38, 104, 77});
    text(v, 1050, 32, 2, WHITE, clock_text);
    rect(v, 1112, 25, 2, 29, (MrbeColor){71, 94, 83});
    /* Wi-Fi arcs and status dot. */
    rect(v, 1131, 28, 5, 2, SUB);
    rect(v, 1137, 33, 5, 2, SUB);
    rect(v, 1143, 38, 5, 2, SUB);
    disk(v, 1143, 47, 3, GREEN);
    rect(v, 1161, 25, 2, 29, (MrbeColor){71, 94, 83});
    disk(v, 1217, 40, 17, (MrbeColor){17, 53, 40});
    outline(v, 1200, 23, 35, 35, (MrbeColor){34, 199, 133});
    disk(v, 1217, 35, 6, GOLD);
    roundrect(v, 1208, 43, 18, 9, 5, (MrbeColor){204, 215, 207});
}

static void page_heading(View *v, const char *title, const char *subtitle)
{
    text(v, MAIN_X + 12, 102, 3, WHITE, title);
    text(v, MAIN_X + 14, 137, 1, SUB, subtitle);
}

static void draw_hero(View *v, const MrbeUIState *s)
{
    int x = SIDEBAR_W, y = 0, w = CANVAS_W - SIDEBAR_W, h = 340;
    if (home_hero.pixels)
        draw_image(v, &home_hero, x, y, w, h, BG);
    else {
        rect(v, x, y, w, h, (MrbeColor){7, 39, 30});
        disk(v, 880, 120, 115, (MrbeColor){31, 77, 56});
        text(v, 218, 105, 2, WHITE, "EXPEDICAO");
        text(v, 218, 138, 4, WHITE, "HORIZONTE");
    }

    /* Focus outlines sit on top of the original MRBE hero artwork. */
    if (s->focus == MRBE_FOCUS_HERO && s->hero_index == 0)
        outline(v, 198, 247, 176, 48, (MrbeColor){75, 255, 183});
    if (s->focus == MRBE_FOCUS_HERO && s->hero_index == 1)
        outline(v, 381, 247, 180, 48, (MrbeColor){75, 255, 183});
}

static void section_title(View *v, int x, int y, const char *title)
{
    rect(v, x, y - 2, 4, 25, GREEN);
    text(v, x + 15, y, 2, WHITE, title);
}

static int home_app_map(int index)
{
    static const int map[9] = {11, 5, 6, 0, 2, 1, 3, 4, -1};
    if (index < 0 || index >= 9) return -1;
    return map[index];
}

static void home_content(View *v, const MrbeUIState *s)
{
    int i;
    roundrect(v, 178, 350, 1084, 190, 24, (MrbeColor){10, 37, 29});
    outline(v, 178, 350, 1084, 190, (MrbeColor){35, 76, 57});
    section_title(v, 195, 365, "RECOMENDADOS PARA VOCE");
    for (i = 0; i < 5; ++i) {
        int x = 190 + i * 211;
        int focused = s->focus == MRBE_FOCUS_HOME_CONTENT &&
                      s->home_content_index == i;
        if (focused)
            outline(v, x - 4, 397, 208, 132, (MrbeColor){65, 255, 180});
        else if (i == 0)
            outline(v, x - 2, 399, 204, 129, (MrbeColor){25, 161, 116});
        if (home_cards[i].pixels)
            draw_image(v, &home_cards[i], x, 401, 200, 125, SURFACE);
        else {
            roundrect(v, x, 401, 200, 125, 16, SURFACE2);
            center(v, x + 100, 450, 1, WHITE, CONTENT_NAMES[i]);
        }
    }
}

static void home_apps(View *v, const MrbeUIState *s)
{
    int i;
    section_title(v, 184, 556, "MEUS APLICATIVOS");
    for (i = 0; i < 9; ++i) {
        int x = 184 + i * 120;
        int focused = s->focus == MRBE_FOCUS_HOME_APPS &&
                      s->home_app_index == i;
        if (focused)
            outline(v, x - 4, 587, 120, 83, (MrbeColor){65, 255, 180});
        else if (i == 0)
            outline(v, x - 2, 589, 116, 79, (MrbeColor){25, 161, 116});
        if (app_tiles[i].pixels)
            draw_image(v, &app_tiles[i], x, 591, 112, 75, SURFACE);
        else {
            roundrect(v, x, 591, 112, 75, 12, APP_COLORS[i < 4 ? i : 11]);
            center(v, x + 56, 620, 1, WHITE, i == 8 ? "TODOS" : APP_NAMES[i < 4 ? i : 4]);
        }
    }
}

static void home_screen(View *v, const MrbeUIState *s)
{
    draw_hero(v, s);
    home_content(v, s);
    home_apps(v, s);
}

static void app_logo(View *v, int app, int x, int y, int w, int h)
{
    MrbeColor c = APP_COLORS[app];
    roundrect(v, x, y, w, h, 13, c);
    if (app == 0) center(v, x + w / 2, y + h / 2 - 8, 3, WHITE, "N");
    else if (app == 1) center(v, x + w / 2, y + h / 2 - 6, 2, WHITE, "D+");
    else if (app == 2) center(v, x + w / 2, y + h / 2 - 6, 2, WHITE, "PRIME");
    else if (app == 3) center(v, x + w / 2, y + h / 2 - 6, 3, WHITE, "MAX");
    else center(v, x + w / 2, y + h / 2 - 6, 2, WHITE,
                app == 4 ? "YT" : app == 5 ? "KODI" : app == 6 ? "VLC" :
                app == 7 ? "FILE" : app == 8 ? "WEB" :
                app == 9 ? "MUS" : app == 10 ? "SYS" : "MRBE");
}

static void content_art(View *v, int x, int y, int w, int h, int variant)
{
    MrbeColor a = (MrbeColor){(unsigned char)(12 + variant * 3),
                              (unsigned char)(44 + variant * 5),
                              (unsigned char)(42 + variant * 2)};
    MrbeColor b = (MrbeColor){(unsigned char)(43 + variant * 4),
                              (unsigned char)(83 + variant * 3),
                              (unsigned char)(62 + variant * 2)};
    int i;
    for (i = 0; i < h; i += 7) {
        int t = i * 255 / h;
        MrbeColor c = {
            (unsigned char)((a.r * (255 - t) + b.r * t) / 255),
            (unsigned char)((a.g * (255 - t) + b.g * t) / 255),
            (unsigned char)((a.b * (255 - t) + b.b * t) / 255)
        };
        rect(v, x, y + i, w, mini(7, h - i), c);
    }
    disk(v, x + w - 35, y + 31, 20 + variant % 3 * 4, GOLD);
    rect(v, x + 8, y + h - 38, w - 16, 30, (MrbeColor){10, 24, 20});
}

static void draw_category_tabs(View *v, const MrbeUIState *s, int store)
{
    int i, x = MAIN_X + 12;
    const char **labels = store ? STORE_CATEGORIES :
        (const char *[]){"TODOS", "FAVORITOS", "RECENTES"};
    int count = store ? 4 : 3;
    for (i = 0; i < count; ++i) {
        int width = store ? (i == 0 ? 116 : i == 1 ? 116 : i == 2 ? 91 : 144) :
                            (i == 0 ? 88 : i == 1 ? 116 : 100);
        int active = s->category_index == i;
        if (active && s->focus == MRBE_FOCUS_CATEGORIES)
            outline(v, x - 2, 169, width + 4, 34, GOLD);
        roundrect(v, x, 171, width, 30, 10,
                  active ? (MrbeColor){27, 83, 58} : SURFACE);
        center(v, x + width / 2, 182, 1, active ? WHITE : SUB, labels[i]);
        x += width + 10;
    }
}

static int store_app_index(const MrbeUIState *s, int visible)
{
    if (s->category_index == 1) return visible;
    if (s->category_index == 2) return visible + 4;
    if (s->category_index == 3) return visible + 7;
    return visible;
}

static int store_count(const MrbeUIState *s)
{
    if (s->category_index == 1) return 4;
    if (s->category_index == 2) return 3;
    if (s->category_index == 3) return 5;
    return MRBE_APP_COUNT;
}

static void app_card(View *v, int app, int x, int y, int w, int h, int focused, int store)
{
    roundrect(v, x, y, w, h, 16, (MrbeColor){20, 33, 29});
    if (focused) outline(v, x - 3, y - 3, w + 6, h + 6, GOLD);
    app_logo(v, app, x + 13, y + 12, 66, 62);
    text(v, x + 91, y + 21, 1, WHITE, APP_NAMES[app]);
    text(v, x + 91, y + 45, 1, SUB, store ? "VER DETALHES" : "APLICATIVO");
    if (app < 4) {
        roundrect(v, x + 91, y + 72, 95, 19, 7, (MrbeColor){68, 54, 30});
        center(v, x + 138, y + 78, 1, GOLD, "REQUER DRM");
    } else {
        roundrect(v, x + 91, y + 72, 95, 19, 7, (MrbeColor){28, 70, 50});
        center(v, x + 138, y + 78, 1, GREEN, "CATALOGO");
    }
}

static int visible_app_count(const MrbeUIState *s);
static int app_index_for_grid(const MrbeUIState *s, int visible);

static void grid_screen(View *v, const MrbeUIState *s, int store)
{
    int i, count = store ? store_count(s) : visible_app_count(s);
    int y0 = 232, card_w = 226, card_h = 112, gap_x = 17, gap_y = 24;
    page_heading(v, store ? "MRBE STORE" : "MEUS APLICATIVOS",
                 store ? "CATALOGO DE APLICATIVOS E RECURSOS" :
                         "SELECIONE UM APLICATIVO PARA VER INFORMACOES");
    draw_category_tabs(v, s, store);
    for (i = 0; i < count; ++i) {
        int app = store ? store_app_index(s, i) : app_index_for_grid(s, i);
        int col = i % 4, row = i / 4;
        int x = MAIN_X + 12 + col * (card_w + gap_x);
        int y = y0 + row * (card_h + gap_y);
        int focused = s->focus == MRBE_FOCUS_GRID && s->grid_index == i;
        app_card(v, app, x, y, card_w, card_h, focused, store);
    }
    text(v, MAIN_X + 14, 682, 1, SUB,
         "ENTER: DETALHES   F: FAVORITO   ESC: VOLTAR   A: APPS");
}

static void draw_library(View *v, const MrbeUIState *s)
{
    int i, x0 = MAIN_X + 12, y = 248, shown = 0;
    int items[MRBE_APP_COUNT];
    page_heading(v, "SUA BIBLIOTECA", "FAVORITOS E APLICATIVOS VISUALIZADOS NESTA SESSAO");
    draw_category_tabs(v, s, 0);
    for (i = 0; i < MRBE_APP_COUNT; ++i) {
        int include = s->category_index == 1 ?
                      (s->favorite_mask & (1u << i)) != 0 :
                      s->category_index == 2 ?
                      (s->viewed_mask & (1u << i)) != 0 :
                      ((s->favorite_mask | s->viewed_mask) & (1u << i)) != 0;
        if (include) items[shown++] = i;
    }
    if (shown == 0) {
        roundrect(v, MAIN_X + 80, 260, 800, 270, 24, (MrbeColor){19, 36, 29});
        disk(v, MAIN_X + 480, 350, 34, (MrbeColor){35, 74, 53});
        center(v, MAIN_X + 480, 343, 3, GOLD, "B");
        center(v, MAIN_X + 480, 421, 2, WHITE, "SUA BIBLIOTECA ESTA VAZIA");
        center(v, MAIN_X + 480, 454, 1, SUB, "USE F EM UM APLICATIVO PARA ADICIONAR AOS FAVORITOS");
    } else {
        for (i = 0; i < shown && i < MRBE_APP_COUNT; ++i) {
            int col = i % 4, row = i / 4;
            int x = x0 + col * 241, yy = y + row * 145;
            app_card(v, items[i], x, yy, 226, 112,
                     s->focus == MRBE_FOCUS_GRID && s->grid_index == i, 0);
        }
    }
    text(v, MAIN_X + 14, 682, 1, SUB, "ENTER: DETALHES   F: REMOVER/ADICIONAR FAVORITO");
}

static void draw_details(View *v, const MrbeUIState *s)
{
    int app = s->detail_app_index;
    if (app < 0 || app >= MRBE_APP_COUNT) app = 0;
    page_heading(v, "DETALHES DO APLICATIVO", "INFORMACOES E COMPATIBILIDADE");
    roundrect(v, MAIN_X + 14, 178, 960, 420, 22, (MrbeColor){17, 32, 27});
    app_logo(v, app, MAIN_X + 50, 220, 180, 150);
    text(v, MAIN_X + 270, 225, 3, WHITE, APP_NAMES[app]);
    text(v, MAIN_X + 272, 270, 1, SUB, APP_DESCRIPTIONS[app]);
    rect(v, MAIN_X + 270, 301, 620, 2, GOLD);
    text(v, MAIN_X + 270, 332, 1, GOLD, "PLATAFORMA");
    text(v, MAIN_X + 270, 355, 1, WHITE, app < 4 ? "ANDROID TV CERTIFICADO / LOJA OFICIAL" : "MRBE LINUX / QEMU");
    text(v, MAIN_X + 270, 395, 1, GOLD, "STATUS NESTA VERSAO");
    text(v, MAIN_X + 270, 418, 1, WHITE, app < 4 ? "NAO INSTALADO - REQUER DRM E CERTIFICACAO" : "ENTRADA DE CATALOGO DEMONSTRATIVA");
    text(v, MAIN_X + 270, 458, 1, SUB, "O QEMU ATUAL NAO DISTRIBUI APLICATIVOS PROPRIETARIOS.");
    if (s->detail_action == 0) outline(v, MAIN_X + 49, 522, 190, 45, GOLD);
    roundrect(v, MAIN_X + 52, 525, 184, 39, 12, (MrbeColor){33, 79, 55});
    center(v, MAIN_X + 144, 538, 1, WHITE, "ABRIR / INSTALAR");
    if (s->detail_action == 1) outline(v, MAIN_X + 252, 522, 164, 45, GOLD);
    roundrect(v, MAIN_X + 255, 525, 158, 39, 12, SURFACE2);
    center(v, MAIN_X + 334, 538, 1, WHITE,
           (s->favorite_mask & (1u << app)) ? "REMOVER F" : "ADICIONAR F");
    if (s->detail_action == 2) outline(v, MAIN_X + 430, 522, 130, 45, GOLD);
    roundrect(v, MAIN_X + 433, 525, 124, 39, 12, SURFACE);
    center(v, MAIN_X + 495, 538, 1, SUB, "VOLTAR");
}

static void draw_films_family(View *v, const MrbeUIState *s, const char *title, const char *subtitle)
{
    int i, x0 = MAIN_X + 12, y = 236;
    page_heading(v, title, subtitle);
    for (i = 0; i < 8; ++i) {
        int col = i % 4, row = i / 4;
        int x = x0 + col * 241, yy = y + row * 190;
        int focused = s->focus == MRBE_FOCUS_GRID && s->grid_index == i;
        if (focused) outline(v, x - 3, yy - 3, 226, 163, GOLD);
        roundrect(v, x, yy, 220, 157, 15, SURFACE);
        content_art(v, x + 4, yy + 4, 212, 149, i + 2);
        text(v, x + 13, yy + 125, 1, WHITE, CONTENT_NAMES[i]);
    }
    text(v, MAIN_X + 14, 682, 1, SUB, "CONTEUDO ILUSTRATIVO - NENHUM SERVICO DE STREAMING CONECTADO");
}

static void draw_settings(View *v, const MrbeUIState *s)
{
    int i;
    page_heading(v, "CONFIGURACOES", "CONTROLE E PERSONALIZE SUA EXPERIENCIA MRBE");
    roundrect(v, MAIN_X + 12, 178, 332, 470, 19, (MrbeColor){17, 31, 26});
    roundrect(v, MAIN_X + 365, 178, 610, 470, 19, (MrbeColor){18, 36, 29});
    for (i = 0; i < 8; ++i) {
        int y = 194 + i * 55;
        if (s->settings_index == i)
            roundrect(v, MAIN_X + 24, y, 306, 43, 11, (MrbeColor){30, 79, 55});
        text(v, MAIN_X + 42, y + 14, 1,
             s->settings_index == i ? WHITE : SUB, SETTINGS_NAMES[i]);
    }
    text(v, MAIN_X + 399, 218, 2, GOLD, "INFORMACOES");
    text(v, MAIN_X + 399, 265, 2, WHITE, SETTINGS_NAMES[s->settings_index]);
    rect(v, MAIN_X + 399, 297, 520, 2, GOLD);
    switch (s->settings_index) {
        case 0:
            text(v, MAIN_X + 399, 335, 1, WHITE, "REDE VIRTUAL QEMU / DHCP");
            text(v, MAIN_X + 399, 365, 1, SUB, "CONSULTE O CONSOLE PARA VER O ENDERECO IP");
            break;
        case 1:
            text(v, MAIN_X + 399, 335, 1, WHITE, "FRAMEBUFFER / RESOLUCAO DA JANELA QEMU");
            text(v, MAIN_X + 399, 365, 1, SUB, "A RESOLUCAO E DEFINIDA PELO DISPLAY VIRTUAL");
            break;
        case 2:
            text(v, MAIN_X + 399, 335, 1, WHITE, "AUDIO: INTEGRACAO PENDENTE");
            text(v, MAIN_X + 399, 365, 1, SUB, "O PERFIL ATUAL NAO CONFIGURA MIXER");
            break;
        case 3:
            text(v, MAIN_X + 399, 335, 1, WHITE, "TECLADO VIRTIO / EVDEV");
            text(v, MAIN_X + 399, 365, 1, SUB, "SETAS, ENTER, ESC, H, A, S, Q, M E F");
            break;
        case 4:
            text(v, MAIN_X + 399, 335, 1, WHITE, "CATALOGO MRBE DE DEMONSTRACAO");
            text(v, MAIN_X + 399, 365, 1, SUB, "INSTALACAO DE PACOTES AINDA NAO HABILITADA");
            break;
        case 5:
            text(v, MAIN_X + 399, 335, 1, WHITE, "ROOTFS EXT4 / BUILDROOT");
            text(v, MAIN_X + 399, 365, 1, SUB, "IMAGEM QEMU EXECUTADA EM MODO SNAPSHOT");
            break;
        case 6:
            text(v, MAIN_X + 399, 335, 1, WHITE, "ATUALIZACOES OTA: NAO IMPLEMENTADAS");
            text(v, MAIN_X + 399, 365, 1, SUB, "NAO HA ATUALIZACAO AUTOMATICA NESTE PROTOTIPO");
            break;
        default: {
            FILE *f = fopen("/proc/meminfo", "r");
            char line[128];
            unsigned long total_kb = 0, avail_kb = 0;
            if (f) {
                while (fgets(line, sizeof(line), f)) {
                    if (sscanf(line, "MemTotal: %lu kB", &total_kb) == 1) continue;
                    if (sscanf(line, "MemAvailable: %lu kB", &avail_kb) == 1) continue;
                }
                fclose(f);
            }
            text(v, MAIN_X + 399, 335, 1, WHITE, "MRBE TV OS 0.7.0 / LINUX ARM64");
            if (total_kb) {
                char mem[64];
                snprintf(mem, sizeof(mem), "RAM TOTAL: %lu MB", total_kb / 1024);
                text(v, MAIN_X + 399, 365, 1, SUB, mem);
                snprintf(mem, sizeof(mem), "RAM DISPONIVEL: %lu MB", avail_kb / 1024);
                text(v, MAIN_X + 399, 389, 1, SUB, mem);
            }
            text(v, MAIN_X + 399, 430, 1, SUB, "BUILDROOT 2025.02.19 / KERNEL 6.12.27");
            break;
        }
    }
    roundrect(v, MAIN_X + 399, 540, 520, 54, 12, (MrbeColor){29, 62, 44});
    text(v, MAIN_X + 420, 561, 1, WHITE, "ENTER: VER DETALHES   ESC: VOLTAR");
}

static void draw_search(View *v, const MrbeUIState *s)
{
    int i;
    page_heading(v, "PESQUISAR", "ENCONTRE APLICATIVOS E CATEGORIAS");
    roundrect(v, MAIN_X + 12, 180, 950, 66, 16, (MrbeColor){24, 42, 34});
    draw_search_icon(v, MAIN_X + 47, 213, GOLD);
    text(v, MAIN_X + 78, 206, 2, WHITE, "PESQUISA POR TEXTO EM DESENVOLVIMENTO");
    text(v, MAIN_X + 14, 285, 2, WHITE, "EXPLORE POR CATEGORIA");
    for (i = 0; i < 4; ++i) {
        int x = MAIN_X + 12 + i * 241;
        roundrect(v, x, 330, 220, 112, 15, SURFACE);
        if (s->search_index == i) outline(v, x - 3, 327, 226, 118, GOLD);
        center(v, x + 110, 367, 2, GOLD,
               i == 0 ? "STREAMING" : i == 1 ? "FILMES" : i == 2 ? "SERIES" : "UTILITARIOS");
        center(v, x + 110, 401, 1, SUB, "EXPLORAR");
    }
}

static void draw_quick_panel(View *v, const MrbeUIState *s)
{
    static const char *items[] = {"REDE / DHCP", "TELA / 1080P", "SOM / PENDENTE", "SISTEMA / ECONOMIA"};
    int i;
    roundrect(v, 806, 82, 430, 300, 20, (MrbeColor){14, 29, 23});
    outline(v, 806, 82, 430, 300, (MrbeColor){65, 111, 79});
    text(v, 835, 110, 2, WHITE, "PAINEL RAPIDO");
    for (i = 0; i < 4; ++i) {
        int y = 157 + i * 48;
        roundrect(v, 831, y, 378, 38, 10,
                  s->quick_index == i ? (MrbeColor){36, 91, 60} : SURFACE);
        text(v, 851, y + 13, 1, s->quick_index == i ? WHITE : SUB, items[i]);
    }
    text(v, 835, 360, 1, SUB, "ESC OU M: FECHAR");
}

static void notification(View *v, const MrbeUIState *s)
{
    int len = (int)strlen(s->notice);
    if (len < 1 || !s->show_notice) return;
    roundrect(v, MAIN_X + 120, 650, 740, 48, 14, (MrbeColor){37, 66, 53});
    rect(v, MAIN_X + 120, 650, 4, 48, GOLD);
    text(v, MAIN_X + 145, 668, len > 75 ? 1 : 1, WHITE, s->notice);
}

static void set_notice(MrbeUIState *s, const char *message)
{
    snprintf(s->notice, sizeof(s->notice), "%s", message);
    s->show_notice = 1;
}

static void open_screen(MrbeUIState *s, MrbeScreen screen, int nav)
{
    s->screen = screen;
    s->nav_index = nav;
    if (screen == MRBE_SCREEN_SETTINGS) s->focus = MRBE_FOCUS_SETTINGS;
    else if (screen == MRBE_SCREEN_HOME) s->focus = MRBE_FOCUS_NAV;
    else if (screen == MRBE_SCREEN_SEARCH) s->focus = MRBE_FOCUS_SEARCH;
    else if (screen == MRBE_SCREEN_APPS || screen == MRBE_SCREEN_STORE ||
             screen == MRBE_SCREEN_LIBRARY) s->focus = MRBE_FOCUS_CATEGORIES;
    else s->focus = MRBE_FOCUS_GRID;
    s->grid_index = 0;
    s->category_index = 0;
}

static void open_details(MrbeUIState *s, int app)
{
    if (app < 0 || app >= MRBE_APP_COUNT) return;
    s->return_screen = s->screen;
    s->return_focus = s->focus;
    s->detail_app_index = app;
    s->detail_action = 0;
    s->viewed_mask |= (1u << app);
    s->screen = MRBE_SCREEN_DETAILS;
    s->focus = MRBE_FOCUS_DETAIL_ACTIONS;
}

static int visible_app_count(const MrbeUIState *s)
{
    int count = 0, i;
    if (s->screen == MRBE_SCREEN_APPS && s->category_index != 0) {
        for (i = 0; i < MRBE_APP_COUNT; ++i) {
            if (s->category_index == 1 && (s->favorite_mask & (1u << i))) ++count;
            else if (s->category_index == 2 && (s->viewed_mask & (1u << i))) ++count;
        }
        return count;
    }
    if (s->screen == MRBE_SCREEN_LIBRARY) {
        for (i = 0; i < MRBE_APP_COUNT; ++i) {
            if (s->category_index == 1 && (s->favorite_mask & (1u << i))) ++count;
            else if (s->category_index == 2 && (s->viewed_mask & (1u << i))) ++count;
            else if (s->category_index == 0 &&
                     ((s->favorite_mask | s->viewed_mask) & (1u << i))) ++count;
        }
        return count;
    }
    if (s->screen == MRBE_SCREEN_STORE) return store_count(s);
    return MRBE_APP_COUNT;
}

static int library_app_at(const MrbeUIState *s, int visible)
{
    int i, n = 0;
    for (i = 0; i < MRBE_APP_COUNT; ++i) {
        int include = s->category_index == 1 ? (s->favorite_mask & (1u << i)) != 0 :
                      s->category_index == 2 ? (s->viewed_mask & (1u << i)) != 0 :
                      ((s->favorite_mask | s->viewed_mask) & (1u << i)) != 0;
        if (include) {
            if (n == visible) return i;
            ++n;
        }
    }
    return 0;
}

static int app_index_for_grid(const MrbeUIState *s, int visible)
{
    int i, n = 0;
    if (s->screen == MRBE_SCREEN_STORE) return store_app_index(s, visible);
    if (s->screen == MRBE_SCREEN_LIBRARY) return library_app_at(s, visible);
    if (s->screen == MRBE_SCREEN_APPS && s->category_index != 0) {
        for (i = 0; i < MRBE_APP_COUNT; ++i) {
            int include = s->category_index == 1 ?
                          (s->favorite_mask & (1u << i)) != 0 :
                          (s->viewed_mask & (1u << i)) != 0;
            if (include) {
                if (n == visible) return i;
                ++n;
            }
        }
        return 0;
    }
    return visible;
}

static int current_grid_app(const MrbeUIState *s)
{
    return app_index_for_grid(s, s->grid_index);
}

static void toggle_favorite(MrbeUIState *s, int app)
{
    if (app < 0 || app >= MRBE_APP_COUNT) return;
    s->favorite_mask ^= (1u << app);
    set_notice(s, (s->favorite_mask & (1u << app)) ?
               "APLICATIVO ADICIONADO AOS FAVORITOS" :
               "APLICATIVO REMOVIDO DOS FAVORITOS");
}

void mrbe_ui_init(MrbeUIState *s)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->screen = MRBE_SCREEN_HOME;
    s->return_screen = MRBE_SCREEN_APPS;
    s->focus = MRBE_FOCUS_NAV;
    s->nav_index = 0;
    s->detail_app_index = 0;
    s->pointer_x = 640;
    s->pointer_y = 360;
    s->pointer_visible = 0;
    load_ui_assets();
}

void mrbe_ui_handle_key(MrbeUIState *s, int key)
{
    int count, app;
    if (!s) return;
    s->show_notice = 0;

    if (key == MRBE_KEY_MENU) {
        s->quick_panel = !s->quick_panel;
        s->quick_index = 0;
        return;
    }
    if (s->quick_panel) {
        if (key == MRBE_KEY_BACK || key == MRBE_KEY_MENU) {
            s->quick_panel = 0;
        } else if (key == MRBE_KEY_UP && s->quick_index > 0) --s->quick_index;
        else if (key == MRBE_KEY_DOWN && s->quick_index < 3) ++s->quick_index;
        else if (key == MRBE_KEY_ENTER) {
            set_notice(s, "PAINEL RAPIDO: CONTROLES DEPENDEM DO HARDWARE");
            s->quick_panel = 0;
        }
        return;
    }

    if (key == MRBE_KEY_HOME) {
        open_screen(s, MRBE_SCREEN_HOME, 0);
        return;
    }
    if (key == MRBE_KEY_APPS) {
        open_screen(s, MRBE_SCREEN_APPS, 2);
        return;
    }
    if (key == MRBE_KEY_SETTINGS) {
        open_screen(s, MRBE_SCREEN_SETTINGS, 6);
        return;
    }
    if (key == MRBE_KEY_SEARCH) {
        open_screen(s, MRBE_SCREEN_SEARCH, 4);
        return;
    }
    if (key == MRBE_KEY_BACK) {
        if (s->screen == MRBE_SCREEN_DETAILS) {
            s->screen = s->return_screen;
            s->focus = s->return_focus;
        } else if (s->screen != MRBE_SCREEN_HOME) {
            open_screen(s, MRBE_SCREEN_HOME, 0);
        } else if (s->focus != MRBE_FOCUS_NAV) {
            s->focus = MRBE_FOCUS_NAV;
        } else {
            set_notice(s, "MRBE TV OS 0.7.0");
        }
        return;
    }

    if (s->screen == MRBE_SCREEN_DETAILS) {
        if (key == MRBE_KEY_LEFT && s->detail_action > 0) --s->detail_action;
        else if (key == MRBE_KEY_RIGHT && s->detail_action < 2) ++s->detail_action;
        else if (key == MRBE_KEY_FAVORITE) toggle_favorite(s, s->detail_app_index);
        else if (key == MRBE_KEY_ENTER) {
            if (s->detail_action == 1) toggle_favorite(s, s->detail_app_index);
            else if (s->detail_action == 0) {
                if (s->detail_app_index < 4)
                    set_notice(s, "STREAMING OFICIAL REQUER ANDROID TV COMPATIVEL E DRM");
                else
                    set_notice(s, "APLICATIVO AINDA NAO INSTALADO NESTE PROTOTIPO LINUX");
            }
            else {
                s->screen = s->return_screen;
                s->focus = s->return_focus;
            }
        }
        return;
    }

    if (s->screen == MRBE_SCREEN_SETTINGS) {
        if (key == MRBE_KEY_UP && s->settings_index > 0) --s->settings_index;
        else if (key == MRBE_KEY_DOWN && s->settings_index < 7) ++s->settings_index;
        else if (key == MRBE_KEY_ENTER)
            set_notice(s, "DETALHES DO SISTEMA EXIBIDOS NO PAINEL");
        return;
    }

    if (s->screen == MRBE_SCREEN_HOME) {
        if (key == MRBE_KEY_FAVORITE) {
            if (s->focus == MRBE_FOCUS_HOME_APPS) {
                int app = home_app_map(s->home_app_index);
                if (app >= 0) toggle_favorite(s, app);
            }
            return;
        }
        if (s->focus == MRBE_FOCUS_NAV) {
            if (key == MRBE_KEY_UP && s->nav_index > 0) --s->nav_index;
            else if (key == MRBE_KEY_DOWN && s->nav_index < 6) ++s->nav_index;
            else if (key == MRBE_KEY_RIGHT) s->focus = MRBE_FOCUS_HERO;
            else if (key == MRBE_KEY_ENTER) {
                switch (s->nav_index) {
                    case 0: open_screen(s, MRBE_SCREEN_HOME, 0); break;
                    case 1: open_screen(s, MRBE_SCREEN_FILMS, 1); break;
                    case 2: open_screen(s, MRBE_SCREEN_APPS, 2); break;
                    case 3: open_screen(s, MRBE_SCREEN_STORE, 3); break;
                    case 4: open_screen(s, MRBE_SCREEN_SEARCH, 4); break;
                    case 5: open_screen(s, MRBE_SCREEN_LIBRARY, 5); break;
                    default: open_screen(s, MRBE_SCREEN_SETTINGS, 6); break;
                }
            }
        } else if (s->focus == MRBE_FOCUS_HERO) {
            if (key == MRBE_KEY_UP) s->focus = MRBE_FOCUS_NAV;
            else if (key == MRBE_KEY_LEFT) s->hero_index = 0;
            else if (key == MRBE_KEY_RIGHT) s->hero_index = 1;
            else if (key == MRBE_KEY_DOWN) s->focus = MRBE_FOCUS_HOME_CONTENT;
            else if (key == MRBE_KEY_ENTER) {
                if (s->hero_index == 0) open_screen(s, MRBE_SCREEN_FILMS, 1);
                else open_screen(s, MRBE_SCREEN_STORE, 3);
            }
        } else if (s->focus == MRBE_FOCUS_HOME_APPS) {
            if (key == MRBE_KEY_LEFT && s->home_app_index > 0) --s->home_app_index;
            else if (key == MRBE_KEY_RIGHT && s->home_app_index < 8) ++s->home_app_index;
            else if (key == MRBE_KEY_UP) s->focus = MRBE_FOCUS_HOME_CONTENT;
            else if (key == MRBE_KEY_ENTER) {
                int selected_app = home_app_map(s->home_app_index);
                if (selected_app >= 0) open_details(s, selected_app);
                else open_screen(s, MRBE_SCREEN_APPS, 2);
            }
        } else if (s->focus == MRBE_FOCUS_HOME_CONTENT) {
            if (key == MRBE_KEY_LEFT && s->home_content_index > 0) --s->home_content_index;
            else if (key == MRBE_KEY_RIGHT && s->home_content_index < 4) ++s->home_content_index;
            else if (key == MRBE_KEY_UP) s->focus = MRBE_FOCUS_HERO;
            else if (key == MRBE_KEY_DOWN) s->focus = MRBE_FOCUS_HOME_APPS;
            else if (key == MRBE_KEY_ENTER)
                set_notice(s, "CONTEUDO ILUSTRATIVO - SERVICO DE STREAMING NAO CONECTADO");
        }
        return;
    }

    if (s->screen == MRBE_SCREEN_SEARCH) {
        if (key == MRBE_KEY_LEFT && s->search_index > 0) --s->search_index;
        else if (key == MRBE_KEY_RIGHT && s->search_index < 3) ++s->search_index;
        else if (key == MRBE_KEY_ENTER) {
            open_screen(s, s->search_index == 0 ? MRBE_SCREEN_STORE :
                           s->search_index == 1 ? MRBE_SCREEN_FILMS :
                           s->search_index == 2 ? MRBE_SCREEN_SERIES : MRBE_SCREEN_APPS,
                        s->search_index == 0 ? 3 : s->search_index == 1 ? 1 :
                        s->search_index == 2 ? 1 : 2);
        }
        return;
    }

    if (s->screen == MRBE_SCREEN_APPS || s->screen == MRBE_SCREEN_STORE ||
        s->screen == MRBE_SCREEN_LIBRARY) {
        if (key == MRBE_KEY_FAVORITE && s->focus == MRBE_FOCUS_GRID) {
            toggle_favorite(s, current_grid_app(s));
            return;
        }
        if (s->focus == MRBE_FOCUS_CATEGORIES) {
            int max_cat = s->screen == MRBE_SCREEN_STORE ? 3 : 2;
            if (key == MRBE_KEY_LEFT && s->category_index > 0) {
                --s->category_index; s->grid_index = 0;
            } else if (key == MRBE_KEY_RIGHT && s->category_index < max_cat) {
                ++s->category_index; s->grid_index = 0;
            } else if (key == MRBE_KEY_DOWN) {
                s->focus = MRBE_FOCUS_GRID;
            } else if (key == MRBE_KEY_ENTER) {
                s->focus = MRBE_FOCUS_GRID;
            }
        } else {
            count = visible_app_count(s);
            if (key == MRBE_KEY_LEFT && s->grid_index % 4 > 0) --s->grid_index;
            else if (key == MRBE_KEY_RIGHT && s->grid_index % 4 < 3 &&
                     s->grid_index + 1 < count) ++s->grid_index;
            else if (key == MRBE_KEY_UP) {
                if (s->grid_index >= 4) s->grid_index -= 4;
                else s->focus = MRBE_FOCUS_CATEGORIES;
            } else if (key == MRBE_KEY_DOWN && s->grid_index + 4 < count) {
                s->grid_index += 4;
            } else if (key == MRBE_KEY_ENTER && count > 0) {
                app = current_grid_app(s);
                open_details(s, app);
            }
        }
        return;
    }

    if (s->screen == MRBE_SCREEN_FILMS || s->screen == MRBE_SCREEN_SERIES ||
        s->screen == MRBE_SCREEN_SPORTS || s->screen == MRBE_SCREEN_KIDS) {
        if (key == MRBE_KEY_LEFT && s->grid_index % 4 > 0) --s->grid_index;
        else if (key == MRBE_KEY_RIGHT && s->grid_index < 7 && s->grid_index % 4 < 3) ++s->grid_index;
        else if (key == MRBE_KEY_UP && s->grid_index >= 4) s->grid_index -= 4;
        else if (key == MRBE_KEY_DOWN && s->grid_index < 4) s->grid_index += 4;
        else if (key == MRBE_KEY_ENTER)
            set_notice(s, "CARTAO DE CONTEUDO DEMONSTRATIVO - SEM REPRODUCAO");
    }
}



static int pointer_in(int x, int y, int left, int top, int width, int height)
{
    return x >= left && y >= top && x < left + width && y < top + height;
}

static int pointer_nav_index(int x, int y)
{
    int i;
    if (x < 5 || x > 158) return -1;
    for (i = 0; i < 7; ++i) {
        int top = 166 + i * 62;
        if (y >= top && y < top + 47) return i;
    }
    return -1;
}

static int pointer_category_index(int x, int y, int store)
{
    int i, left = MAIN_X + 12;
    int count = store ? 4 : 3;
    if (y < 169 || y >= 205) return -1;
    for (i = 0; i < count; ++i) {
        int width = store ? (i == 0 ? 116 : i == 1 ? 116 : i == 2 ? 91 : 144) :
                            (i == 0 ? 88 : i == 1 ? 116 : 100);
        if (x >= left && x < left + width) return i;
        left += width + 10;
    }
    return -1;
}

static void pointer_activate_nav(MrbeUIState *s, int index, int clicked)
{
    if (index < 0 || index > 6) return;
    s->nav_index = index;
    if (!clicked) return; /* Hover highlights; only a click changes screen. */
    s->screen = MRBE_SCREEN_HOME;
    s->focus = MRBE_FOCUS_NAV;
    mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
}

void mrbe_ui_handle_pointer(MrbeUIState *s, int x, int y, int clicked)
{
    int nav, index, col, row, count;
    if (!s) return;

    if (x < 0) x = 0;
    if (x >= CANVAS_W) x = CANVAS_W - 1;
    if (y < 0) y = 0;
    if (y >= CANVAS_H) y = CANVAS_H - 1;
    s->pointer_x = x;
    s->pointer_y = y;
    s->pointer_visible = 1;
    s->show_notice = 0;

    if (s->quick_panel) {
        if (pointer_in(x, y, 831, 157, 378, 38)) s->quick_index = 0;
        else if (pointer_in(x, y, 831, 205, 378, 38)) s->quick_index = 1;
        else if (pointer_in(x, y, 831, 253, 378, 38)) s->quick_index = 2;
        else if (pointer_in(x, y, 831, 301, 378, 38)) s->quick_index = 3;
        else if (clicked) {
            mrbe_ui_handle_key(s, MRBE_KEY_BACK);
            return;
        }
        if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
        return;
    }

    nav = pointer_nav_index(x, y);
    if (nav >= 0) {
        pointer_activate_nav(s, nav, clicked);
        return;
    }

    switch (s->screen) {
        case MRBE_SCREEN_HOME:
            if (pointer_in(x, y, 198, 247, 176, 48)) {
                s->focus = MRBE_FOCUS_HERO;
                s->hero_index = 0;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            } else if (pointer_in(x, y, 381, 247, 180, 48)) {
                s->focus = MRBE_FOCUS_HERO;
                s->hero_index = 1;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            } else if (pointer_in(x, y, 190, 397, 1048, 132)) {
                s->focus = MRBE_FOCUS_HOME_CONTENT;
                index = (x - 190) / 211;
                if (index < 0) index = 0;
                if (index > 4) index = 4;
                s->home_content_index = index;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            } else if (pointer_in(x, y, 184, 587, 1080, 83)) {
                s->focus = MRBE_FOCUS_HOME_APPS;
                index = (x - 184) / 120;
                if (index < 0) index = 0;
                if (index > 8) index = 8;
                s->home_app_index = index;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            } else if (pointer_in(x, y, SIDEBAR_W, 0, CANVAS_W - SIDEBAR_W, 340)) {
                s->focus = MRBE_FOCUS_HERO;
                s->hero_index = x > 430 ? 1 : 0;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            }
            break;

        case MRBE_SCREEN_APPS:
        case MRBE_SCREEN_STORE:
        case MRBE_SCREEN_LIBRARY:
            index = pointer_category_index(x, y, s->screen == MRBE_SCREEN_STORE);
            if (index >= 0) {
                s->focus = MRBE_FOCUS_CATEGORIES;
                s->category_index = index;
                s->grid_index = 0;
                if (clicked) s->focus = MRBE_FOCUS_GRID;
                break;
            }
            if (s->screen == MRBE_SCREEN_LIBRARY) {
                col = (x - (MAIN_X + 12)) / 241;
                row = (y - 248) / 145;
                if (x >= MAIN_X + 12 && y >= 248 && col >= 0 && col < 4 && row >= 0) {
                    index = row * 4 + col;
                    count = visible_app_count(s);
                    if (index < count) {
                        s->focus = MRBE_FOCUS_GRID;
                        s->grid_index = index;
                        if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
                    }
                }
            } else {
                col = (x - (MAIN_X + 12)) / 243;
                row = (y - 232) / 136;
                if (x >= MAIN_X + 12 && y >= 232 && col >= 0 && col < 4 && row >= 0) {
                    index = row * 4 + col;
                    count = s->screen == MRBE_SCREEN_STORE ? store_count(s) : visible_app_count(s);
                    if (index < count) {
                        s->focus = MRBE_FOCUS_GRID;
                        s->grid_index = index;
                        if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
                    }
                }
            }
            break;

        case MRBE_SCREEN_DETAILS:
            if (pointer_in(x, y, MAIN_X + 49, 522, 190, 45)) s->detail_action = 0;
            else if (pointer_in(x, y, MAIN_X + 252, 522, 164, 45)) s->detail_action = 1;
            else if (pointer_in(x, y, MAIN_X + 430, 522, 130, 45)) s->detail_action = 2;
            else break;
            s->focus = MRBE_FOCUS_DETAIL_ACTIONS;
            if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            break;

        case MRBE_SCREEN_SETTINGS:
            if (x >= MAIN_X + 24 && x < MAIN_X + 330 && y >= 194 && y < 194 + 8 * 55) {
                index = (y - 194) / 55;
                if (index > 7) index = 7;
                s->settings_index = index;
                s->focus = MRBE_FOCUS_SETTINGS;
                if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
            }
            break;

        case MRBE_SCREEN_SEARCH:
            if (y >= 327 && y < 445 && x >= MAIN_X + 12) {
                index = (x - (MAIN_X + 12)) / 241;
                if (index >= 0 && index < 4) {
                    s->search_index = index;
                    s->focus = MRBE_FOCUS_SEARCH;
                    if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
                }
            }
            break;

        case MRBE_SCREEN_FILMS:
        case MRBE_SCREEN_SERIES:
        case MRBE_SCREEN_SPORTS:
        case MRBE_SCREEN_KIDS:
            if (x >= MAIN_X + 12 && y >= 236) {
                col = (x - (MAIN_X + 12)) / 241;
                row = (y - 236) / 190;
                if (col >= 0 && col < 4 && row >= 0 && row < 2) {
                    s->grid_index = row * 4 + col;
                    s->focus = MRBE_FOCUS_GRID;
                    if (clicked) mrbe_ui_handle_key(s, MRBE_KEY_ENTER);
                }
            }
            break;

        default:
            break;
    }
}

static void draw_pointer(View *v, const MrbeUIState *s)
{
    int row;
    static const int widths[19] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 8, 9, 10, 11, 12, 13
    };
    if (!s->pointer_visible) return;

    /* Small gold pointer with a dark outline, designed for both mouse and TV UI. */
    for (row = 0; row < 19; ++row) {
        int width = widths[row];
        rect(v, s->pointer_x - 1, s->pointer_y + row, width + 2, 1,
             (MrbeColor){3, 8, 6});
        if (width > 2)
            rect(v, s->pointer_x + 1, s->pointer_y + row, width - 2, 1, GOLD);
        else
            rect(v, s->pointer_x, s->pointer_y + row, width, 1, WHITE);
    }
}

void mrbe_ui_draw(MrbeFramebuffer *fb, const MrbeUIState *s)
{
    View v;
    if (!fb || !s) return;
    v.fb = fb;
    v.w = (int)fb->variable.xres;
    v.h = (int)fb->variable.yres;
    {
        float scale_x = (float)v.w / (float)CANVAS_W;
        float scale_y = (float)v.h / (float)CANVAS_H;
        v.scale = scale_x < scale_y ? scale_x : scale_y;
        if (v.scale <= 0.0f) v.scale = 1.0f;
        v.offset_x = (v.w - (int)((float)CANVAS_W * v.scale + 0.5f)) / 2;
        v.offset_y = (v.h - (int)((float)CANVAS_H * v.scale + 0.5f)) / 2;
    }
    background(&v);
    sidebar(&v, s);

    switch (s->screen) {
        case MRBE_SCREEN_HOME:
            home_screen(&v, s);
            break;
        case MRBE_SCREEN_APPS:
            grid_screen(&v, s, 0);
            break;
        case MRBE_SCREEN_STORE:
            grid_screen(&v, s, 1);
            break;
        case MRBE_SCREEN_LIBRARY:
            draw_library(&v, s);
            break;
        case MRBE_SCREEN_DETAILS:
            draw_details(&v, s);
            break;
        case MRBE_SCREEN_SETTINGS:
            draw_settings(&v, s);
            break;
        case MRBE_SCREEN_SEARCH:
            draw_search(&v, s);
            break;
        case MRBE_SCREEN_FILMS:
            draw_films_family(&v, s, "FILMES", "EXPLORE CATEGORIAS E DESTAQUES");
            break;
        case MRBE_SCREEN_SERIES:
            draw_films_family(&v, s, "SERIES", "SERIES E PRODUCOES EM DESTAQUE");
            break;
        case MRBE_SCREEN_SPORTS:
            draw_films_family(&v, s, "ESPORTES", "CONTEUDO ESPORTIVO DEMONSTRATIVO");
            break;
        case MRBE_SCREEN_KIDS:
            draw_films_family(&v, s, "INFANTIL", "UMA AREA PARA TODA A FAMILIA");
            break;
        default:
            home_screen(&v, s);
            break;
    }
    top_status(&v);
    if (s->quick_panel) draw_quick_panel(&v, s);
    notification(&v, s);
    if (s->screen != MRBE_SCREEN_HOME) text(&v, MAIN_X + 12, 690, 1, (MrbeColor){90, 116, 103},
         "H HOME  A APPS  S CONFIG  Q BUSCA  M PAINEL  ESC VOLTAR");
    draw_pointer(&v, s);
}
