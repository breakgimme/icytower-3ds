#include <stdio.h>

#include "menu.h"
#include "icytower.h"
#include "gfx.h"
#include "sfx.h"
#include "options.h"
#include "characters.h"
#include "floor_types.h"
#include "fullscreen.h"

enum {
	MAIN_MENU, OPTIONS, GAME_OPTIONS, GFX_OPTIONS, SOUND_OPTIONS, CONTROLS
} menu_page = MAIN_MENU;

unsigned int menu_bullet = 0;

void menu_up(void) {
	play_sample(sample_menu_choose, volume_sfx / 10.0);
	if (menu_bullet != 0)
		menu_bullet -= 1;
	else switch (menu_page) {
	case MAIN_MENU:
		menu_bullet = 3;
		break;
	case OPTIONS:
		menu_bullet = 4;
		break;
	case GAME_OPTIONS:
	case GFX_OPTIONS:
	case SOUND_OPTIONS:
		menu_bullet = 2;
		break;
	case CONTROLS:
		menu_bullet = 5;
		break;
	}
}

void menu_down(void) {
	play_sample(sample_menu_choose, volume_sfx / 10.0);
	menu_bullet += 1;
	switch (menu_page) {
	case MAIN_MENU:
		menu_bullet %= 4;
		break;
	case OPTIONS:
		menu_bullet %= 5;
		break;
	case GAME_OPTIONS:
	case GFX_OPTIONS:
	case SOUND_OPTIONS:
		menu_bullet %= 3;
		break;
	case CONTROLS:
		menu_bullet %= 6;
		break;
	}
}

void menu_enter(void) {
	switch (menu_page) {
	case MAIN_MENU:
		switch (menu_bullet) {
		case 0:
			/* START GAME */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			game_state = PLAYING;
			start_game();
			break;
		case 1:
			/* INSTRUCTIONS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			game_state = INSTRUCTIONS;
			break;
		case 2:
			/* OPTIONS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 0;
			break;
		case 3:
			/* EXIT */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			game_state = EXIT;
			break;
		}
		break;
	case OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* GAME OPTIONS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = GAME_OPTIONS;
			menu_bullet = 0;
			break;
		case 1:
			/* GFX OPTIONS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = GFX_OPTIONS;
			menu_bullet = 0;
			break;
		case 2:
			/* SOUND OPTIONS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = SOUND_OPTIONS;
			menu_bullet = 0;
			break;
		case 3:
			/* CONTROLS */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = CONTROLS;
			menu_bullet = 0;
			break;
		case 4:
			/* BACK */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = MAIN_MENU;
			menu_bullet = 2;
			break;
		}
		break;
	case GAME_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* CHARACTER */
			break;
		case 1:
			/* START FLOOR */
			break;
		case 2:
			/* BACK */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 0;
			break;
		}
		break;
	case GFX_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* EYE CANDY */
			break;
		case 1:
			/* FULLSCREEN */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			fullscreen = !fullscreen;
			if (fullscreen)
				enable_fullscreen();
			else
				disable_fullscreen();
			break;
		case 2:
			/* BACK */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 1;
			break;
		}
		break;
	case SOUND_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* SOUND */
			break;
		case 1:
			/* MUSIC */
			break;
		case 2:
			/* BACK */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 2;
			break;
		}
		break;
	case CONTROLS:
		switch (menu_bullet) {
		case 0:
			/* LEFT */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			printf("Changing controls is not implemented yet\n");
			break;
		case 1:
			/* RIGHT */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			printf("Changing controls is not implemented yet\n");
			break;
		case 2:
			/* JUMP */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			printf("Changing controls is not implemented yet\n");
			break;
		case 3:
			/* PAUSE */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			printf("Changing controls is not implemented yet\n");
			break;
		case 4:
			/* REJUMP */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			rejump = !rejump;
			break;
		case 5:
			/* BACK */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 3;
			break;
		}
		break;
	}
}

void menu_left(void) {
	switch (menu_page) {
	case GAME_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* CHARACTER */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (character_index > 0)
				--character_index;
			break;
		case 1:
			/* START FLOOR */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (start_floor > 0)
				--start_floor;
			break;
		}
		break;
	case GFX_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* EYE CANDY */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (eye_candy > 0)
				--eye_candy;
			break;
		}
		break;
	case SOUND_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* SOUND */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (volume_sfx > 0)
				--volume_sfx;
			break;
		case 1:
			/* MUSIC */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (volume_music > 0)
				--volume_music;
			set_music_gain(volume_music / 10.0f);
			break;
		}
		break;
	default:
		break;
	}
}

void menu_right(void) {
	switch (menu_page) {
	case GAME_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* CHARACTER */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (character_index < characters_count - 1)
				++character_index;
			break;
		case 1:
			/* START FLOOR */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (start_floor < floor_types_count - 1)
				++start_floor;
			break;
		}
		break;
	case GFX_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* EYE CANDY */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (eye_candy < 2)
				++eye_candy;
			break;
		}
		break;
	case SOUND_OPTIONS:
		switch (menu_bullet) {
		case 0:
			/* SOUND */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (volume_sfx < 10)
				++volume_sfx;
			break;
		case 1:
			/* MUSIC */
			play_sample(sample_menu_change, volume_sfx / 10.0);
			if (volume_music < 10)
				++volume_music;
			set_music_gain(volume_music / 10.0f);
			break;
		}
		break;
	default:
		break;
	}
}

void menu_escape(void) {
	switch (menu_page) {
	case MAIN_MENU:
		if (menu_bullet == 3) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			game_state = EXIT;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 3;
		}
		break;
	case OPTIONS:
		if (menu_bullet == 4) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = MAIN_MENU;
			menu_bullet = 2;
			break;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 4;
		}
		break;
	case GAME_OPTIONS:
		if (menu_bullet == 2) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 0;
			break;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 2;
		}
		break;
	case GFX_OPTIONS:
		if (menu_bullet == 2) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 1;
			break;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 2;
		}
		break;
	case SOUND_OPTIONS:
		if (menu_bullet == 2) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 2;
			break;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 2;
		}
		break;
	case CONTROLS:
		if (menu_bullet == 5) {
			play_sample(sample_menu_change, volume_sfx / 10.0);
			menu_page = OPTIONS;
			menu_bullet = 3;
			break;
		} else {
			play_sample(sample_menu_choose, volume_sfx / 10.0);
			menu_bullet = 5;
		}
		break;
	}
}

const char *get_volume_bar(unsigned int n) {
	static char volume_bar[11];
	unsigned int i;
	for (i = 0; i < 10; ++i)
		volume_bar[i] = i < n ? '}' : '{';
	return volume_bar;
}

void draw_menu(void) {
	if (fullscreen)
		screen_clear(COLOR_BLACK);
	draw_image(bitmap_title_bg, 0, 0);
	draw_image(bitmap_title, 250, 20);
	draw_image(bitmap_menu_bullet, 4, 262 + 28 * menu_bullet);
	switch (menu_page) {
	case MAIN_MENU:
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "START GAME");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "INSTRUCTIONS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "OPTIONS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 3, ALIGN_LEFT,  "EXIT");
		break;
	case OPTIONS:
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "GAME OPTIONS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "GFX OPTIONS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "SOUND OPTIONS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 3, ALIGN_LEFT,  "CONTROLS");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 4, ALIGN_LEFT,  "BACK");
		break;
	case GAME_OPTIONS:
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "CHARACTER:");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "START FLOOR:");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "BACK");
		draw_image(characters[character_index].gfx.idle1, 330, 256);
		draw_image(floor_types[start_floor].left, 315, 308);
		draw_image(floor_types[start_floor].mid, 336, 308);
		draw_image(floor_types[start_floor].right, 352, 308);
		break;
	case GFX_OPTIONS:
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "EYE CANDY: %s",
				eye_candy == 0 ? "NONE" :
				eye_candy == 1 ? "SOME" :
				eye_candy == 2 ? "LOTS" :
				"");
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "STRETCH: %s",
				fullscreen ? "YES" : "NO");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "BACK");
		break;
	case SOUND_OPTIONS:
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "SOUND:%s",
				get_volume_bar(volume_sfx));
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "MUSIC:%s",
				get_volume_bar(volume_music));
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "BACK");
		break;
	case CONTROLS:
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 0, ALIGN_LEFT,  "LEFT:  (%s)",
				KEY_NAME_LEFT);
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 1, ALIGN_LEFT,  "RIGHT: (%s)",
				KEY_NAME_RIGHT);
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 2, ALIGN_LEFT,  "JUMP:  (%s)",
				KEY_NAME_JUMP);
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 3, ALIGN_LEFT,  "PAUSE: (%s)",
				KEY_NAME_PAUSE);
		draw_textf(font_color, COLOR_WHITE, 40, 270 + 28 * 4, ALIGN_LEFT,  "REJUMP: %s",
				rejump ? "YES" : "NO");
		draw_text(font_color, COLOR_WHITE, 40, 270 + 28 * 5, ALIGN_LEFT,  "BACK");
		break;
	}
}

void draw_instructions(void) {
	if (fullscreen)
		screen_clear(COLOR_BLACK);
	draw_image(bitmap_title_bg, 0, 0);
	draw_image(bitmap_instructions, 0, 0);
}
