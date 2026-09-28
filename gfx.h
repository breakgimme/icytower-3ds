#ifndef GFX_3DS_H
#define GFX_3DS_H

#include <stdbool.h>
#include <stdint.h>

#include <3ds.h>
#include <citro2d.h>
#include <tex3ds.h>

typedef struct {
    int w, h;
    C3D_Tex tex;
    Tex3DS_SubTexture sub;
    C2D_Image img;
    C3D_Tex flipTex;
    Tex3DS_SubTexture flipSub;
    C2D_Image flipImg;
    bool hasFlip;
} Image;

typedef struct {
    Image *glyph[96];
    int height;
} BitmapFont;

typedef uint32_t Color;
#define RGB(r, g, b) \
    ((((uint32_t)(r)) << 24) | (((uint32_t)(g)) << 16) | \
     (((uint32_t)(b)) << 8) | 0xFFu)
#define COLOR_WHITE RGB(255, 255, 255)
#define COLOR_BLACK RGB(0, 0, 0)

enum {
    ALIGN_LEFT = 0,
    ALIGN_CENTER = 1,
    ALIGN_RIGHT = 2
};

#define SCREEN_W 640
#define SCREEN_H 480

void gfx_3ds_init(void);

void screen_top_begin(void);
void screen_bottom_begin(void);
void draw_bottom_brick(void);

Image *load_image(const char *path);
void free_image(Image *img);

void screen_clear(Color c);
void draw_image(const Image *img, int dx, int dy);
void draw_image_flipped(const Image *img, int dx, int dy);
void draw_image_region(const Image *img, int sx, int sy, int sw, int sh,
                       int dx, int dy);
void draw_image_pivot(const Image *img, int pivot_x, int pivot_y,
                      int dx, int dy, float angle_rad);

int text_width(const BitmapFont *font, const char *text);
void draw_text(const BitmapFont *font, Color color, int x, int y, int align,
               const char *text);
void draw_textf(const BitmapFont *font, Color color, int x, int y, int align,
                const char *fmt, ...);

void screen_present(int stretch);

extern Image *bitmap_bgtile;
extern Image *bitmap_browse_button;
extern Image *bitmap_bt_no;
extern Image *bitmap_bt_yes;
extern Image *bitmap_clock;
extern Image *bitmap_clock_hand;
extern Image *bitmap_combo_count;
extern Image *bitmap_combo_liquid;
extern Image *bitmap_combo_meter;
extern Image *bitmap_floor01;
extern Image *bitmap_floor02;
extern Image *bitmap_floor03;
extern Image *bitmap_floor04;
extern Image *bitmap_floor05;
extern Image *bitmap_floor06;
extern Image *bitmap_floor07;
extern Image *bitmap_floor08;
extern Image *bitmap_floor09;
extern Image *bitmap_floor10;
extern Image *bitmap_floor11;
extern Image *bitmap_floor12;
extern Image *bitmap_floor12a;
extern Image *bitmap_floor12b;
extern Image *bitmap_floor12c;
extern Image *bitmap_floor13;
extern Image *bitmap_floor14;
extern Image *bitmap_floor15;
extern Image *bitmap_floor16;
extern Image *bitmap_floor17;
extern Image *bitmap_floor18;
extern Image *bitmap_floor18a;
extern Image *bitmap_floor18b;
extern Image *bitmap_floor18c;
extern Image *bitmap_floor19;
extern Image *bitmap_floor20;
extern Image *bitmap_floor21;
extern Image *bitmap_floor22;
extern Image *bitmap_floor23;
extern Image *bitmap_floor24;
extern Image *bitmap_floor25;
extern Image *bitmap_floor26;
extern Image *bitmap_floor27;
extern Image *bitmap_gameover;
extern Image *bitmap_heroface000;
extern Image *bitmap_heroface001;
extern Image *bitmap_heroface002;
extern Image *bitmap_highscore;
extern Image *bitmap_hint;
extern Image *bitmap_hisctop;
extern Image *bitmap_hurryup;
extern Image *bitmap_instructions;
extern Image *bitmap_menu_bullet;
extern Image *bitmap_replay_bg;
extern Image *bitmap_replay_buttons;
extern Image *bitmap_reward000;
extern Image *bitmap_reward001;
extern Image *bitmap_reward002;
extern Image *bitmap_reward003;
extern Image *bitmap_reward004;
extern Image *bitmap_reward005;
extern Image *bitmap_reward006;
extern Image *bitmap_reward007;
extern Image *bitmap_reward008;
extern Image *bitmap_reward009;
extern Image *bitmap_sideblock;
extern Image *bitmap_sign01;
extern Image *bitmap_sign02;
extern Image *bitmap_sign03;
extern Image *bitmap_sign04;
extern Image *bitmap_sign04a;
extern Image *bitmap_sign05;
extern Image *bitmap_sign06;
extern Image *bitmap_sign06a;
extern Image *bitmap_sign07;
extern Image *bitmap_sign08;
extern Image *bitmap_sign09;
extern Image *bitmap_sort_button_all;
extern Image *bitmap_sort_button_combo;
extern Image *bitmap_sort_button_floor;
extern Image *bitmap_sort_button_name;
extern Image *bitmap_sort_button_score;
extern Image *bitmap_star01;
extern Image *bitmap_star02;
extern Image *bitmap_star03;
extern Image *bitmap_star04;
extern Image *bitmap_star05;
extern Image *bitmap_star06;
extern Image *bitmap_star07;
extern Image *bitmap_star08;
extern Image *bitmap_title;
extern Image *bitmap_title_bg;
extern Image *bitmap_vcr;
extern Image *bitmap_vcr_left;
extern Image *bitmap_vcr_right;
extern Image *bitmap_vcr_up;

extern Image *bitmap_harold_chock;
extern Image *bitmap_harold_edge1;
extern Image *bitmap_harold_edge2;
extern Image *bitmap_harold_idle1;
extern Image *bitmap_harold_idle2;
extern Image *bitmap_harold_idle3;
extern Image *bitmap_harold_jump;
extern Image *bitmap_harold_jump1;
extern Image *bitmap_harold_jump2;
extern Image *bitmap_harold_jump3;
extern Image *bitmap_harold_rotate;
extern Image *bitmap_harold_walk1;
extern Image *bitmap_harold_walk2;
extern Image *bitmap_harold_walk3;
extern Image *bitmap_harold_walk4;

extern Image *bitmap_disco_dave_chock;
extern Image *bitmap_disco_dave_edge1;
extern Image *bitmap_disco_dave_edge2;
extern Image *bitmap_disco_dave_idle1;
extern Image *bitmap_disco_dave_idle2;
extern Image *bitmap_disco_dave_idle3;
extern Image *bitmap_disco_dave_jump;
extern Image *bitmap_disco_dave_jump1;
extern Image *bitmap_disco_dave_jump2;
extern Image *bitmap_disco_dave_jump3;
extern Image *bitmap_disco_dave_rotate;
extern Image *bitmap_disco_dave_walk1;
extern Image *bitmap_disco_dave_walk2;
extern Image *bitmap_disco_dave_walk3;
extern Image *bitmap_disco_dave_walk4;

extern BitmapFont *font_color;
extern BitmapFont *font_mono;
extern BitmapFont *font_native;

bool gfx_load_bitmaps(void);
void gfx_destroy_bitmaps(void);
bool gfx_load_fonts(void);
void gfx_destroy_fonts(void);

#endif
