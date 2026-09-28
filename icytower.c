#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <3ds.h>

#include "icytower.h"
#include "gfx.h"
#include "sfx.h"
#include "menu.h"
#include "options.h"
#include "characters.h"
#include "floor_types.h"
#include "game.h"
#include "physics.h"

#define TICK_MS 20

enum GAME_STATE game_state;

static void clear_bottom_screen(void) {
    int i;
    for (i = 0; i < 2; ++i) {
        u16 w = 0, h = 0;
        uint16_t *fb = (uint16_t *)gfxGetFramebuffer(GFX_BOTTOM,
                                                    GFX_LEFT, &w, &h);
        size_t n = (w != 0 && h != 0) ? (size_t)w * h : (size_t)240 * 320;
        size_t j;
        for (j = 0; j < n; ++j)
            fb[j] = 0;
        gfxFlushBuffers();
        gfxSwapBuffers();
    }
    gspWaitForVBlank();
}

static void panic(void) {
    screen_top_begin();
    screen_clear(RGB(255, 0, 0));
    screen_present(0);
    gspWaitForVBlank();
    while (aptMainLoop()) {
        gspWaitForVBlank();
        hidScanInput();
        if (hidKeysDown() & KEY_START)
            break;
    }
}

void start_game(void) {
    stop_music(characters[character_index].sfx.bgmusic);
    play_music(characters[character_index].sfx.bgmusic);
    initialize_game();
    draw_game();
    play_sample(characters[character_index].sfx.greeting,
                volume_sfx / 10.0f);
}

static void pause_game(void) {
    play_sample(characters[character_index].sfx.pause,
                volume_sfx / 10.0f);
}

static void to_main_menu(void) {
    stop_music(characters[character_index].sfx.bgmusic);
    play_music(audio_stream_bg_menu);
}

#define MENU_DIRS (KEY_DUP | KEY_DDOWN | KEY_DLEFT | KEY_DRIGHT)

static u32 menu_dir_state(u32 down, u32 held) {
    static u64 rep_at[4];
    static u32 prev_held;
    static u32 prev_cdir;
    static const u32 bits[4] = {
        KEY_DUP, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT
    };
    circlePosition cpos;
    u32 cdir = 0, h, d, out = 0;
    u64 now;
    int i;

    hidCircleRead(&cpos);
    if (cpos.dy > 30)
        cdir |= KEY_DUP;
    if (cpos.dy < -30)
        cdir |= KEY_DDOWN;
    if (cpos.dx < -30)
        cdir |= KEY_DLEFT;
    if (cpos.dx > 30)
        cdir |= KEY_DRIGHT;

    h = (held & MENU_DIRS) | cdir;
    d = (down & MENU_DIRS) | (cdir & ~prev_cdir);
    prev_cdir = cdir;
    now = osGetTime();

    for (i = 0; i < 4; ++i) {
        if (d & bits[i]) {
            out |= bits[i];
            rep_at[i] = now + 350;
        } else if (h & bits[i]) {
            if (!(prev_held & bits[i]))
                rep_at[i] = now + 350;
            else if (now >= rep_at[i]) {
                out |= bits[i];
                rep_at[i] = now + 90;
            }
        }
    }
    prev_held = h;
    return out;
}

static void handle_menu_buttons(u32 down, u32 held) {
    u32 mdir = menu_dir_state(down, held);
    if (!down && !mdir)
        return;
    if (!down)
        return;
    switch (game_state) {
    case TITLE:
        if (mdir & KEY_DUP)
            menu_up();
        if (mdir & KEY_DDOWN)
            menu_down();
        if (down & (KEY_A | KEY_START))
            menu_enter();
        if (down & KEY_B)
            menu_escape();
        if (mdir & KEY_DLEFT)
            menu_left();
        if (mdir & KEY_DRIGHT)
            menu_right();
        break;
    case INSTRUCTIONS:
        if (down & (KEY_A | KEY_B | KEY_START | KEY_SELECT))
            game_state = TITLE;
        break;
    case PLAYING:
        if (down & KEY_START) {
            game_state = PAUSE;
            pause_game();
        } else if (down & KEY_SELECT) {
            game_state = ESCAPE;
            pause_game();
        }
        break;
    case PAUSE:

        game_state = PLAYING;
        break;
    case ESCAPE:
        if (down & (KEY_B | KEY_START | KEY_SELECT)) {
            play_sample(sample_tryagain, volume_sfx / 10.0f);
            game_state = TITLE;
            to_main_menu();
        } else {
            game_state = PLAYING;
        }
        break;
    case GAMEOVER:
        if (down & (KEY_A | KEY_B | KEY_START | KEY_SELECT)) {
            play_sample(sample_tryagain, volume_sfx / 10.0f);
            game_state = TITLE;
            to_main_menu();
        }
        break;
    case EXIT:
        break;
    }
}

static int sample_play_keys(u32 held) {
    circlePosition cpos;
    int k = 0;
    hidCircleRead(&cpos);
    if ((held & KEY_DLEFT) || cpos.dx < -30)
        k |= KEY_LEFT;
    if ((held & KEY_DRIGHT) || cpos.dx > 30)
        k |= KEY_RIGHT;
    if (held & (KEY_A | KEY_B | KEY_X | KEY_Y))
        k |= KEY_JUMP;
    return k;
}

#ifndef HOST_TEST
int main(void) {
    u64 last_ms;
    int acc_ms = 0;

    gfxInitDefault();
    clear_bottom_screen();
    gfx_3ds_init();

    if (R_FAILED(romfsInit()))
        panic();

    if (!sfx_init())
        panic();
    set_music_gain(volume_music / 10.0f);

    if (!gfx_load_bitmaps())
        panic();
    if (!gfx_load_fonts())
        panic();
    if (!sfx_load_audio_streams_and_samples())
        panic();

    initialize_characters();
    initialize_floor_types();

    game_state = TITLE;
    to_main_menu();

    last_ms = osGetTime();
    while (aptMainLoop()) {
        u64 now_ms;
        int steps;
        u32 held, down;

        hidScanInput();
        held = hidKeysHeld();
        down = hidKeysDown();
        handle_menu_buttons(down, held);

        now_ms = osGetTime();
        acc_ms += (int)(now_ms - last_ms);
        last_ms = now_ms;
        if (acc_ms > 100)
            acc_ms = 100;
        if (acc_ms < 0)
            acc_ms = 0;

        steps = 0;
        while (game_state == PLAYING && acc_ms >= TICK_MS && steps < 5) {
            set_keys(sample_play_keys(held));
            do_tick();
            acc_ms -= TICK_MS;
            ++steps;
        }
        if (game_state != PLAYING)
            acc_ms = 0;
        if (game_state == EXIT)
            break;

        switch (game_state) {
        case TITLE:
            screen_top_begin();
            draw_menu();
            break;
        case INSTRUCTIONS:
            screen_top_begin();
            draw_instructions();
            break;
        case PLAYING:
            screen_top_begin();
            draw_game();
            break;
        case PAUSE:
            screen_top_begin();
            draw_pause();
            break;
        case ESCAPE:
            screen_top_begin();
            draw_escape();
            break;
        case GAMEOVER:
            screen_top_begin();
            draw_gameover();
            break;
        case EXIT:
            break;
        }
        draw_bottom_brick();
        if (game_state == PLAYING)
            draw_bottom_hud();
        screen_present(fullscreen);
    }

    sfx_destroy_audio_streams_and_samples();
    gfx_destroy_fonts();
    gfx_destroy_bitmaps();
    sfx_shutdown();
    romfsExit();
    gfxExit();
    return 0;
}
#endif
