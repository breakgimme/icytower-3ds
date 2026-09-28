#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <3ds.h>
#include <citro2d.h>
#include <tex3ds.h>

#include "gfx.h"
#include "options.h"

static C3D_RenderTarget *tgt_top;
static C3D_RenderTarget *tgt_bottom;
static bool frame_open;
static bool is_bottom;

void gfx_3ds_init(void) {
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    tgt_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    tgt_bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
}

static void ensure_frame(void) {
    if (!frame_open) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(tgt_top, C2D_Color32(0, 0, 0, 255));
        C2D_SceneBegin(tgt_top);
        frame_open = true;
    }
}

static void view(float *sx, float *sy, float *ox) {
    if (is_bottom) {
        *sx = 0.5f;
        *sy = 0.5f;
        *ox = 0.0f;
    } else if (fullscreen) {
        *sx = 400.0f / SCREEN_W;
        *sy = 240.0f / SCREEN_H;
        *ox = 0.0f;
    } else {
        *sx = 0.5f;
        *sy = 0.5f;
        *ox = 40.0f;
    }
}

static uint32_t to_rgba(Color c) {
    unsigned r = (c >> 24) & 0xFFu;
    unsigned g = (c >> 16) & 0xFFu;
    unsigned b = (c >> 8) & 0xFFu;
    return C2D_Color32(r, g, b, 255);
}

Image *load_image(const char *path) {
    FILE *f = fopen(path, "rb");
    Tex3DS_Texture t3x;
    const Tex3DS_SubTexture *sub;
    Image *img;
    if (!f)
        return NULL;
    img = calloc(1, sizeof(*img));
    if (!img) {
        fclose(f);
        return NULL;
    }
    t3x = Tex3DS_TextureImportStdio(f, &img->tex, NULL, false);
    fclose(f);
    if (!t3x) {
        free(img);
        return NULL;
    }
    if (Tex3DS_GetNumSubTextures(t3x) < 1) {
        Tex3DS_TextureFree(t3x);
        C3D_TexDelete(&img->tex);
        free(img);
        return NULL;
    }
    sub = Tex3DS_GetSubTexture(t3x, 0);
    if (!sub) {
        Tex3DS_TextureFree(t3x);
        C3D_TexDelete(&img->tex);
        free(img);
        return NULL;
    }
    img->sub = *sub;
    Tex3DS_TextureFree(t3x);
    C3D_TexSetFilter(&img->tex, GPU_LINEAR, GPU_LINEAR);
    img->w = img->sub.width;
    img->h = img->sub.height;
    img->img.tex = &img->tex;
    img->img.subtex = &img->sub;
    return img;
}

void free_image(Image *img) {
    if (img) {
        C3D_TexDelete(&img->tex);
        if (img->hasFlip)
            C3D_TexDelete(&img->flipTex);
        free(img);
    }
}

static bool load_flip_image(Image *base, const char *path) {
    Image *tmp;
    if (!base)
        return false;
    tmp = load_image(path);
    if (!tmp)
        return false;
    base->flipTex = tmp->tex;
    base->flipSub = tmp->sub;
    base->flipImg.tex = &base->flipTex;
    base->flipImg.subtex = &base->flipSub;
    base->hasFlip = true;
    free(tmp);
    return true;
}

void screen_clear(Color c) {
    ensure_frame();
    if (c != COLOR_BLACK) {
        float sx, sy, ox;
        view(&sx, &sy, &ox);
        C2D_DrawRectSolid(ox, 0.0f, 0.0f, SCREEN_W * sx,
                          SCREEN_H * sy, to_rgba(c));
    }
}

static bool clip_rect(int *dx, int *dy, int *w, int *h) {
    if (*dx < 0) {
        *w += *dx;
        *dx = 0;
    }
    if (*dy < 0) {
        *h += *dy;
        *dy = 0;
    }
    if (*dx + *w > SCREEN_W)
        *w = SCREEN_W - *dx;
    if (*dy + *h > SCREEN_H)
        *h = SCREEN_H - *dy;
    return *w > 0 && *h > 0;
}

static void draw_clipped(const Image *img, const Tex3DS_SubTexture *base,
                         int sx_, int sy_, int sw, int sh, int dx, int dy) {
    Tex3DS_SubTexture sub = *base;
    C2D_Image part;
    float sx, sy, ox, uw, uh;
    int cx = dx, cy = dy, cw = sw, ch = sh;
    if (!clip_rect(&cx, &cy, &cw, &ch))
        return;
    sx_ += cx - dx;
    sy_ += cy - dy;
    ensure_frame();
    view(&sx, &sy, &ox);
    uw = sub.right - sub.left;
    uh = sub.bottom - sub.top;
    sub.left += uw * sx_ / img->w;
    sub.top += uh * sy_ / img->h;
    sub.right = sub.left + uw * cw / img->w;
    sub.bottom = sub.top + uh * ch / img->h;
    sub.width = cw;
    sub.height = ch;
    part.tex = img->img.tex;
    part.subtex = &sub;
    C2D_DrawImageAt(part, ox + cx * sx, cy * sy, 0.0f, NULL, sx, sy);
}

void draw_image(const Image *img, int dx, int dy) {
    if (!img)
        return;
    draw_clipped(img, &img->sub, 0, 0, img->w, img->h, dx, dy);
}

void draw_image_flipped(const Image *img, int dx, int dy) {
    const Tex3DS_SubTexture *base;
    const C3D_Tex *tex;
    Tex3DS_SubTexture sub;
    C2D_Image part;
    float sx, sy, ox;
    int cx = dx, cy = dy, cw, ch;
    if (!img)
        return;
    base = img->hasFlip ? &img->flipSub : &img->sub;
    tex = img->hasFlip ? &img->flipTex : &img->tex;
    cw = img->w;
    ch = img->h;
    if (!clip_rect(&cx, &cy, &cw, &ch))
        return;
    ensure_frame();
    view(&sx, &sy, &ox);
    sub = *base;
    if (img->hasFlip) {
        float uw = sub.right - sub.left;
        float uh = sub.bottom - sub.top;
        int fx = cx - dx;
        int fy = cy - dy;
        sub.left += uw * fx / img->w;
        sub.top += uh * fy / img->h;
        sub.right = sub.left + uw * cw / img->w;
        sub.bottom = sub.top + uh * ch / img->h;
    } else {
        float uw = sub.right - sub.left;
        float uh = sub.bottom - sub.top;
        int fy = cy - dy;
        float u1 = sub.left + uw * (img->w - (cx - dx)) / img->w;
        float u0 = u1 - uw * cw / img->w;
        sub.left = u1;
        sub.right = u0;
        sub.top += uh * fy / img->h;
        sub.bottom = sub.top + uh * ch / img->h;
    }
    sub.width = cw;
    sub.height = ch;
    part.tex = (C3D_Tex *)tex;
    part.subtex = &sub;
    C2D_DrawImageAt(part, ox + cx * sx, cy * sy, 0.0f, NULL, sx, sy);
}

void draw_image_region(const Image *img, int sx_, int sy_, int sw, int sh,
                       int dx, int dy) {
    Tex3DS_SubTexture sub;
    C2D_Image part;
    float sx, sy, ox, uw, uh;
    if (!img || sw <= 0 || sh <= 0)
        return;
    if (sx_ < 0) {
        sw += sx_;
        dx -= sx_;
        sx_ = 0;
    }
    if (sy_ < 0) {
        sh += sy_;
        dy -= sy_;
        sy_ = 0;
    }
    if (sx_ + sw > img->w)
        sw = img->w - sx_;
    if (sy_ + sh > img->h)
        sh = img->h - sy_;
    if (sw <= 0 || sh <= 0)
        return;
    if (dx < 0) {
        sx_ -= dx;
        sw += dx;
        dx = 0;
    }
    if (dy < 0) {
        sy_ -= dy;
        sh += dy;
        dy = 0;
    }
    if (dx + sw > SCREEN_W)
        sw = SCREEN_W - dx;
    if (dy + sh > SCREEN_H)
        sh = SCREEN_H - dy;
    if (sw <= 0 || sh <= 0)
        return;
    ensure_frame();
    view(&sx, &sy, &ox);
    sub = img->sub;
    uw = sub.right - sub.left;
    uh = sub.bottom - sub.top;
    sub.left += uw * sx_ / img->w;
    sub.top += uh * sy_ / img->h;
    sub.right = sub.left + uw * sw / img->w;
    sub.bottom = sub.top + uh * sh / img->h;
    sub.width = sw;
    sub.height = sh;
    part.tex = img->img.tex;
    part.subtex = &sub;
    C2D_DrawImageAt(part, ox + dx * sx, dy * sy, 0.0f, NULL, sx, sy);
}

void draw_image_pivot(const Image *img, int pivot_x, int pivot_y,
                      int dx, int dy, float angle_rad) {
    float sx, sy, ox;
    float cx, cy;
    if (!img)
        return;
    ensure_frame();
    view(&sx, &sy, &ox);
    cx = ox + (dx + ((float)img->w / 2 - pivot_x)) * sx;
    cy = (dy + ((float)img->h / 2 - pivot_y)) * sy;
    C2D_DrawImageAtRotated(img->img, cx, cy, 0.0f, -angle_rad, NULL, sx,
                           sy);
}

int text_width(const BitmapFont *font, const char *text) {
    int w = 0;
    const unsigned char *p = (const unsigned char *)text;
    if (!font)
        return 0;
    while (*p) {
        if (*p >= 0x20 && *p <= 0x7F && font->glyph[*p - 0x20])
            w += font->glyph[*p - 0x20]->w;
        ++p;
    }
    return w;
}

void draw_text(const BitmapFont *font, Color color, int x, int y, int align,
               const char *text) {
    const unsigned char *p;
    (void)color;
    if (!font || !text)
        return;
    if (align == ALIGN_CENTER)
        x -= text_width(font, text) / 2;
    else if (align == ALIGN_RIGHT)
        x -= text_width(font, text);
    p = (const unsigned char *)text;
    while (*p) {
        if (*p >= 0x20 && *p <= 0x7F && font->glyph[*p - 0x20]) {
            draw_image(font->glyph[*p - 0x20], x, y);
            x += font->glyph[*p - 0x20]->w;
        }
        ++p;
    }
}

void draw_textf(const BitmapFont *font, Color color, int x, int y, int align,
                const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    draw_text(font, color, x, y, align, buf);
}

void screen_top_begin(void) {
    ensure_frame();
    C2D_TargetClear(tgt_top, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(tgt_top);
    is_bottom = false;
}

void screen_bottom_begin(void) {
    ensure_frame();
    C2D_TargetClear(tgt_bottom, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(tgt_bottom);
    is_bottom = true;
}

void draw_bottom_brick(void) {
    screen_bottom_begin();
    draw_image(bitmap_title_bg, 0, 0);
}

void screen_present(int stretch) {
    (void)stretch;
    if (frame_open) {
        C3D_FrameEnd(0);
        frame_open = false;
    }
}

Image *bitmap_bgtile;
Image *bitmap_browse_button;
Image *bitmap_bt_no;
Image *bitmap_bt_yes;
Image *bitmap_clock;
Image *bitmap_clock_hand;
Image *bitmap_combo_count;
Image *bitmap_combo_liquid;
Image *bitmap_combo_meter;
Image *bitmap_floor01;
Image *bitmap_floor02;
Image *bitmap_floor03;
Image *bitmap_floor04;
Image *bitmap_floor05;
Image *bitmap_floor06;
Image *bitmap_floor07;
Image *bitmap_floor08;
Image *bitmap_floor09;
Image *bitmap_floor10;
Image *bitmap_floor11;
Image *bitmap_floor12;
Image *bitmap_floor12a;
Image *bitmap_floor12b;
Image *bitmap_floor12c;
Image *bitmap_floor13;
Image *bitmap_floor14;
Image *bitmap_floor15;
Image *bitmap_floor16;
Image *bitmap_floor17;
Image *bitmap_floor18;
Image *bitmap_floor18a;
Image *bitmap_floor18b;
Image *bitmap_floor18c;
Image *bitmap_floor19;
Image *bitmap_floor20;
Image *bitmap_floor21;
Image *bitmap_floor22;
Image *bitmap_floor23;
Image *bitmap_floor24;
Image *bitmap_floor25;
Image *bitmap_floor26;
Image *bitmap_floor27;
Image *bitmap_gameover;
Image *bitmap_heroface000;
Image *bitmap_heroface001;
Image *bitmap_heroface002;
Image *bitmap_highscore;
Image *bitmap_hint;
Image *bitmap_hisctop;
Image *bitmap_hurryup;
Image *bitmap_instructions;
Image *bitmap_menu_bullet;
Image *bitmap_replay_bg;
Image *bitmap_replay_buttons;
Image *bitmap_reward000;
Image *bitmap_reward001;
Image *bitmap_reward002;
Image *bitmap_reward003;
Image *bitmap_reward004;
Image *bitmap_reward005;
Image *bitmap_reward006;
Image *bitmap_reward007;
Image *bitmap_reward008;
Image *bitmap_reward009;
Image *bitmap_sideblock;
Image *bitmap_sign01;
Image *bitmap_sign02;
Image *bitmap_sign03;
Image *bitmap_sign04;
Image *bitmap_sign04a;
Image *bitmap_sign05;
Image *bitmap_sign06;
Image *bitmap_sign06a;
Image *bitmap_sign07;
Image *bitmap_sign08;
Image *bitmap_sign09;
Image *bitmap_sort_button_all;
Image *bitmap_sort_button_combo;
Image *bitmap_sort_button_floor;
Image *bitmap_sort_button_name;
Image *bitmap_sort_button_score;
Image *bitmap_star01;
Image *bitmap_star02;
Image *bitmap_star03;
Image *bitmap_star04;
Image *bitmap_star05;
Image *bitmap_star06;
Image *bitmap_star07;
Image *bitmap_star08;
Image *bitmap_title;
Image *bitmap_title_bg;
Image *bitmap_vcr;
Image *bitmap_vcr_left;
Image *bitmap_vcr_right;
Image *bitmap_vcr_up;

Image *bitmap_harold_chock;
Image *bitmap_harold_edge1;
Image *bitmap_harold_edge2;
Image *bitmap_harold_idle1;
Image *bitmap_harold_idle2;
Image *bitmap_harold_idle3;
Image *bitmap_harold_jump;
Image *bitmap_harold_jump1;
Image *bitmap_harold_jump2;
Image *bitmap_harold_jump3;
Image *bitmap_harold_rotate;
Image *bitmap_harold_walk1;
Image *bitmap_harold_walk2;
Image *bitmap_harold_walk3;
Image *bitmap_harold_walk4;

Image *bitmap_disco_dave_chock;
Image *bitmap_disco_dave_edge1;
Image *bitmap_disco_dave_edge2;
Image *bitmap_disco_dave_idle1;
Image *bitmap_disco_dave_idle2;
Image *bitmap_disco_dave_idle3;
Image *bitmap_disco_dave_jump;
Image *bitmap_disco_dave_jump1;
Image *bitmap_disco_dave_jump2;
Image *bitmap_disco_dave_jump3;
Image *bitmap_disco_dave_rotate;
Image *bitmap_disco_dave_walk1;
Image *bitmap_disco_dave_walk2;
Image *bitmap_disco_dave_walk3;
Image *bitmap_disco_dave_walk4;

#define ASSET_ROOT "romfs:/"

#define LOAD_BITMAP(name) \
    (bitmap_##name = load_image(ASSET_ROOT "gfx/" #name ".t3x"))

#define LOAD_BITMAP_CHARACTER(character, name) \
    (bitmap_##character##_##name = \
         load_image(ASSET_ROOT "gfx/" #character "/" #name ".t3x"))

#define LOAD_FLIP(name) \
    load_flip_image(bitmap_##name, ASSET_ROOT "gfx/" #name "_flip.t3x")

#define LOAD_FLIP_CHARACTER(character, name) \
    load_flip_image(bitmap_##character##_##name, \
                    ASSET_ROOT "gfx/" #character "/" #name "_flip.t3x")

#define DESTROY_BITMAP(name)         \
    do {                             \
        free_image(bitmap_##name);   \
        bitmap_##name = NULL;        \
    } while (0)

#define DESTROY_BITMAP_CHARACTER(character, name)  \
    do {                                           \
        free_image(bitmap_##character##_##name);   \
        bitmap_##character##_##name = NULL;        \
    } while (0)

bool gfx_load_bitmaps(void) {
    if (!LOAD_BITMAP(bgtile))
        goto destroy;
    if (!LOAD_BITMAP(browse_button))
        goto destroy;
    if (!LOAD_BITMAP(bt_no))
        goto destroy;
    if (!LOAD_BITMAP(bt_yes))
        goto destroy;
    if (!LOAD_BITMAP(clock))
        goto destroy;
    if (!LOAD_BITMAP(clock_hand))
        goto destroy;
    if (!LOAD_BITMAP(combo_count))
        goto destroy;
    if (!LOAD_BITMAP(combo_liquid))
        goto destroy;
    if (!LOAD_BITMAP(combo_meter))
        goto destroy;
    if (!LOAD_BITMAP(floor01))
        goto destroy;
    if (!LOAD_BITMAP(floor02))
        goto destroy;
    if (!LOAD_BITMAP(floor03))
        goto destroy;
    if (!LOAD_BITMAP(floor04))
        goto destroy;
    if (!LOAD_BITMAP(floor05))
        goto destroy;
    if (!LOAD_BITMAP(floor06))
        goto destroy;
    if (!LOAD_BITMAP(floor07))
        goto destroy;
    if (!LOAD_BITMAP(floor08))
        goto destroy;
    if (!LOAD_BITMAP(floor09))
        goto destroy;
    if (!LOAD_BITMAP(floor10))
        goto destroy;
    if (!LOAD_BITMAP(floor11))
        goto destroy;
    if (!LOAD_BITMAP(floor12))
        goto destroy;
    if (!LOAD_BITMAP(floor12a))
        goto destroy;
    if (!LOAD_BITMAP(floor12b))
        goto destroy;
    if (!LOAD_BITMAP(floor12c))
        goto destroy;
    if (!LOAD_BITMAP(floor13))
        goto destroy;
    if (!LOAD_BITMAP(floor14))
        goto destroy;
    if (!LOAD_BITMAP(floor15))
        goto destroy;
    if (!LOAD_BITMAP(floor16))
        goto destroy;
    if (!LOAD_BITMAP(floor17))
        goto destroy;
    if (!LOAD_BITMAP(floor18))
        goto destroy;
    if (!LOAD_BITMAP(floor18a))
        goto destroy;
    if (!LOAD_BITMAP(floor18b))
        goto destroy;
    if (!LOAD_BITMAP(floor18c))
        goto destroy;
    if (!LOAD_BITMAP(floor19))
        goto destroy;
    if (!LOAD_BITMAP(floor20))
        goto destroy;
    if (!LOAD_BITMAP(floor21))
        goto destroy;
    if (!LOAD_BITMAP(floor22))
        goto destroy;
    if (!LOAD_BITMAP(floor23))
        goto destroy;
    if (!LOAD_BITMAP(floor24))
        goto destroy;
    if (!LOAD_BITMAP(floor25))
        goto destroy;
    if (!LOAD_BITMAP(floor26))
        goto destroy;
    if (!LOAD_BITMAP(floor27))
        goto destroy;
    if (!LOAD_BITMAP(gameover))
        goto destroy;
    if (!LOAD_BITMAP(heroface000))
        goto destroy;
    if (!LOAD_BITMAP(heroface001))
        goto destroy;
    if (!LOAD_BITMAP(heroface002))
        goto destroy;
    if (!LOAD_BITMAP(highscore))
        goto destroy;
    if (!LOAD_BITMAP(hint))
        goto destroy;
    if (!LOAD_BITMAP(hisctop))
        goto destroy;
    if (!LOAD_BITMAP(hurryup))
        goto destroy;
    if (!LOAD_BITMAP(instructions))
        goto destroy;
    if (!LOAD_BITMAP(menu_bullet))
        goto destroy;
    if (!LOAD_BITMAP(replay_bg))
        goto destroy;
    if (!LOAD_BITMAP(replay_buttons))
        goto destroy;
    if (!LOAD_BITMAP(reward000))
        goto destroy;
    if (!LOAD_BITMAP(reward001))
        goto destroy;
    if (!LOAD_BITMAP(reward002))
        goto destroy;
    if (!LOAD_BITMAP(reward003))
        goto destroy;
    if (!LOAD_BITMAP(reward004))
        goto destroy;
    if (!LOAD_BITMAP(reward005))
        goto destroy;
    if (!LOAD_BITMAP(reward006))
        goto destroy;
    if (!LOAD_BITMAP(reward007))
        goto destroy;
    if (!LOAD_BITMAP(reward008))
        goto destroy;
    if (!LOAD_BITMAP(reward009))
        goto destroy;
    if (!LOAD_BITMAP(sideblock))
        goto destroy;
    if (!LOAD_BITMAP(sign01))
        goto destroy;
    if (!LOAD_BITMAP(sign02))
        goto destroy;
    if (!LOAD_BITMAP(sign03))
        goto destroy;
    if (!LOAD_BITMAP(sign04))
        goto destroy;
    if (!LOAD_BITMAP(sign04a))
        goto destroy;
    if (!LOAD_BITMAP(sign05))
        goto destroy;
    if (!LOAD_BITMAP(sign06))
        goto destroy;
    if (!LOAD_BITMAP(sign06a))
        goto destroy;
    if (!LOAD_BITMAP(sign07))
        goto destroy;
    if (!LOAD_BITMAP(sign08))
        goto destroy;
    if (!LOAD_BITMAP(sign09))
        goto destroy;
    if (!LOAD_BITMAP(sort_button_all))
        goto destroy;
    if (!LOAD_BITMAP(sort_button_combo))
        goto destroy;
    if (!LOAD_BITMAP(sort_button_floor))
        goto destroy;
    if (!LOAD_BITMAP(sort_button_name))
        goto destroy;
    if (!LOAD_BITMAP(sort_button_score))
        goto destroy;
    if (!LOAD_BITMAP(star01))
        goto destroy;
    if (!LOAD_BITMAP(star02))
        goto destroy;
    if (!LOAD_BITMAP(star03))
        goto destroy;
    if (!LOAD_BITMAP(star04))
        goto destroy;
    if (!LOAD_BITMAP(star05))
        goto destroy;
    if (!LOAD_BITMAP(star06))
        goto destroy;
    if (!LOAD_BITMAP(star07))
        goto destroy;
    if (!LOAD_BITMAP(star08))
        goto destroy;
    if (!LOAD_BITMAP(title))
        goto destroy;
    if (!LOAD_BITMAP(title_bg))
        goto destroy;
    if (!LOAD_BITMAP(vcr))
        goto destroy;
    if (!LOAD_BITMAP(vcr_left))
        goto destroy;
    if (!LOAD_BITMAP(vcr_right))
        goto destroy;
    if (!LOAD_BITMAP(vcr_up))
        goto destroy;

    if (!LOAD_BITMAP_CHARACTER(harold, chock))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, edge1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, edge2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, idle1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, idle2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, idle3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, jump))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, jump1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, jump2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, jump3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, rotate))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, walk1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, walk2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, walk3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(harold, walk4))
        goto destroy;

    if (!LOAD_BITMAP_CHARACTER(disco_dave, chock))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, edge1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, edge2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, idle1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, idle2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, idle3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, jump))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, jump1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, jump2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, jump3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, rotate))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, walk1))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, walk2))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, walk3))
        goto destroy;
    if (!LOAD_BITMAP_CHARACTER(disco_dave, walk4))
        goto destroy;

    if (!LOAD_FLIP(sideblock))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, walk1))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, walk2))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, walk3))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, walk4))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, jump1))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, jump2))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(harold, jump3))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, walk1))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, walk2))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, walk3))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, walk4))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, jump1))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, jump2))
        goto destroy;
    if (!LOAD_FLIP_CHARACTER(disco_dave, jump3))
        goto destroy;

    return true;
destroy:
    gfx_destroy_bitmaps();
    return false;
}

void gfx_destroy_bitmaps(void) {
    DESTROY_BITMAP(bgtile);
    DESTROY_BITMAP(browse_button);
    DESTROY_BITMAP(bt_no);
    DESTROY_BITMAP(bt_yes);
    DESTROY_BITMAP(clock);
    DESTROY_BITMAP(clock_hand);
    DESTROY_BITMAP(combo_count);
    DESTROY_BITMAP(combo_liquid);
    DESTROY_BITMAP(combo_meter);
    DESTROY_BITMAP(floor01);
    DESTROY_BITMAP(floor02);
    DESTROY_BITMAP(floor03);
    DESTROY_BITMAP(floor04);
    DESTROY_BITMAP(floor05);
    DESTROY_BITMAP(floor06);
    DESTROY_BITMAP(floor07);
    DESTROY_BITMAP(floor08);
    DESTROY_BITMAP(floor09);
    DESTROY_BITMAP(floor10);
    DESTROY_BITMAP(floor11);
    DESTROY_BITMAP(floor12);
    DESTROY_BITMAP(floor12a);
    DESTROY_BITMAP(floor12b);
    DESTROY_BITMAP(floor12c);
    DESTROY_BITMAP(floor13);
    DESTROY_BITMAP(floor14);
    DESTROY_BITMAP(floor15);
    DESTROY_BITMAP(floor16);
    DESTROY_BITMAP(floor17);
    DESTROY_BITMAP(floor18);
    DESTROY_BITMAP(floor18a);
    DESTROY_BITMAP(floor18b);
    DESTROY_BITMAP(floor18c);
    DESTROY_BITMAP(floor19);
    DESTROY_BITMAP(floor20);
    DESTROY_BITMAP(floor21);
    DESTROY_BITMAP(floor22);
    DESTROY_BITMAP(floor23);
    DESTROY_BITMAP(floor24);
    DESTROY_BITMAP(floor25);
    DESTROY_BITMAP(floor26);
    DESTROY_BITMAP(floor27);
    DESTROY_BITMAP(gameover);
    DESTROY_BITMAP(heroface000);
    DESTROY_BITMAP(heroface001);
    DESTROY_BITMAP(heroface002);
    DESTROY_BITMAP(highscore);
    DESTROY_BITMAP(hint);
    DESTROY_BITMAP(hisctop);
    DESTROY_BITMAP(hurryup);
    DESTROY_BITMAP(instructions);
    DESTROY_BITMAP(menu_bullet);
    DESTROY_BITMAP(replay_bg);
    DESTROY_BITMAP(replay_buttons);
    DESTROY_BITMAP(reward000);
    DESTROY_BITMAP(reward001);
    DESTROY_BITMAP(reward002);
    DESTROY_BITMAP(reward003);
    DESTROY_BITMAP(reward004);
    DESTROY_BITMAP(reward005);
    DESTROY_BITMAP(reward006);
    DESTROY_BITMAP(reward007);
    DESTROY_BITMAP(reward008);
    DESTROY_BITMAP(reward009);
    DESTROY_BITMAP(sideblock);
    DESTROY_BITMAP(sign01);
    DESTROY_BITMAP(sign02);
    DESTROY_BITMAP(sign03);
    DESTROY_BITMAP(sign04);
    DESTROY_BITMAP(sign04a);
    DESTROY_BITMAP(sign05);
    DESTROY_BITMAP(sign06);
    DESTROY_BITMAP(sign06a);
    DESTROY_BITMAP(sign07);
    DESTROY_BITMAP(sign08);
    DESTROY_BITMAP(sign09);
    DESTROY_BITMAP(sort_button_all);
    DESTROY_BITMAP(sort_button_combo);
    DESTROY_BITMAP(sort_button_floor);
    DESTROY_BITMAP(sort_button_name);
    DESTROY_BITMAP(sort_button_score);
    DESTROY_BITMAP(star01);
    DESTROY_BITMAP(star02);
    DESTROY_BITMAP(star03);
    DESTROY_BITMAP(star04);
    DESTROY_BITMAP(star05);
    DESTROY_BITMAP(star06);
    DESTROY_BITMAP(star07);
    DESTROY_BITMAP(star08);
    DESTROY_BITMAP(title);
    DESTROY_BITMAP(title_bg);
    DESTROY_BITMAP(vcr);
    DESTROY_BITMAP(vcr_left);
    DESTROY_BITMAP(vcr_right);
    DESTROY_BITMAP(vcr_up);

    DESTROY_BITMAP_CHARACTER(harold, chock);
    DESTROY_BITMAP_CHARACTER(harold, edge1);
    DESTROY_BITMAP_CHARACTER(harold, edge2);
    DESTROY_BITMAP_CHARACTER(harold, idle1);
    DESTROY_BITMAP_CHARACTER(harold, idle2);
    DESTROY_BITMAP_CHARACTER(harold, idle3);
    DESTROY_BITMAP_CHARACTER(harold, jump);
    DESTROY_BITMAP_CHARACTER(harold, jump1);
    DESTROY_BITMAP_CHARACTER(harold, jump2);
    DESTROY_BITMAP_CHARACTER(harold, jump3);
    DESTROY_BITMAP_CHARACTER(harold, rotate);
    DESTROY_BITMAP_CHARACTER(harold, walk1);
    DESTROY_BITMAP_CHARACTER(harold, walk2);
    DESTROY_BITMAP_CHARACTER(harold, walk3);
    DESTROY_BITMAP_CHARACTER(harold, walk4);

    DESTROY_BITMAP_CHARACTER(disco_dave, chock);
    DESTROY_BITMAP_CHARACTER(disco_dave, edge1);
    DESTROY_BITMAP_CHARACTER(disco_dave, edge2);
    DESTROY_BITMAP_CHARACTER(disco_dave, idle1);
    DESTROY_BITMAP_CHARACTER(disco_dave, idle2);
    DESTROY_BITMAP_CHARACTER(disco_dave, idle3);
    DESTROY_BITMAP_CHARACTER(disco_dave, jump);
    DESTROY_BITMAP_CHARACTER(disco_dave, jump1);
    DESTROY_BITMAP_CHARACTER(disco_dave, jump2);
    DESTROY_BITMAP_CHARACTER(disco_dave, jump3);
    DESTROY_BITMAP_CHARACTER(disco_dave, rotate);
    DESTROY_BITMAP_CHARACTER(disco_dave, walk1);
    DESTROY_BITMAP_CHARACTER(disco_dave, walk2);
    DESTROY_BITMAP_CHARACTER(disco_dave, walk3);
    DESTROY_BITMAP_CHARACTER(disco_dave, walk4);
}

BitmapFont *font_color;
BitmapFont *font_mono;
BitmapFont *font_native;

static BitmapFont *load_font(const char *directory, unsigned first,
                             unsigned last) {
    BitmapFont *font = calloc(1, sizeof(*font));
    char path[256];
    unsigned cp;
    if (!font)
        return NULL;
    for (cp = first; cp <= last; ++cp) {
        Image *g;
        snprintf(path, sizeof(path), ASSET_ROOT "%s%.8x.t3x", directory,
                 cp);
        g = load_image(path);
        if (!g) {
            unsigned i;
            for (i = 0; i < 96; ++i)
                free_image(font->glyph[i]);
            free(font);
            return NULL;
        }
        font->glyph[cp - first] = g;
        if (g->h > font->height)
            font->height = g->h;
    }
    return font;
}

bool gfx_load_fonts(void) {
    font_color = load_font("gfx/font1/", 0x20, 0x7f);
    font_mono = load_font("gfx/font2/", 0x20, 0x7f);
    font_native = load_font("gfx/font3/", 0x20, 0x7f);
    if (!font_color || !font_mono || !font_native) {
        gfx_destroy_fonts();
        return false;
    }
    return true;
}

static void destroy_font(BitmapFont *font) {
    if (font) {
        unsigned i;
        for (i = 0; i < 96; ++i)
            free_image(font->glyph[i]);
        free(font);
    }
}

void gfx_destroy_fonts(void) {
    destroy_font(font_color);
    font_color = NULL;
    destroy_font(font_mono);
    font_mono = NULL;
    destroy_font(font_native);
    font_native = NULL;
}
