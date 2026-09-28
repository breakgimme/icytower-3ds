#include <time.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "game.h"
#include "icytower.h"
#include "gfx.h"
#include "sfx.h"
#include "options.h"
#include "characters.h"
#include "floor_types.h"
#include "physics.h"

IT_STATE it_state;
int keys;

enum {
	ANIMATION_IDLE,
	ANIMATION_CHOCK,
	ANIMATION_WALK_LEFT,
	ANIMATION_WALK_RIGHT,
	ANIMATION_FLY,
	ANIMATION_FLY_LEFT,
	ANIMATION_FLY_RIGHT,
	ANIMATION_ROTATE
} animation;

int animation_frame;

void initialize_game(void) {
	init_state(&it_state, rejump, time(NULL));
	keys = 0;
	animation = ANIMATION_IDLE;
	animation_frame = 0;
}

void set_keys(int k) { keys = k; }

void do_tick(void) {
	int prev_status = it_state.status;
	int prev_floor = it_state.floor;
	int prev_combo_timer = it_state.combo_timer;
	if (!play_frame(&it_state, keys)) {
		play_sample(characters[character_index].sfx.death, volume_sfx / 10.0);
		play_sample(sample_gameover, volume_sfx / 10.0);
		game_state = GAMEOVER;
		return;
	}

	if (it_state.speed_counter == 1500) {
		play_sample(sample_ring, volume_sfx / 10.0);
		play_sample(sample_hurryup, volume_sfx / 10.0);
	}

	if (it_state.floor <= 1000 ? it_state.floor / 50 > prev_floor / 50 :
			it_state.floor / 500 > prev_floor / 500)
		play_sample(sample_aight, volume_sfx / 10.0);

	if (prev_combo_timer > 0 && it_state.combo_timer == 0
			&& it_state.combo_count > 1) {
		if (it_state.combo_floor >= 200)
			play_sample(sample_unbelievable, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 140)
			play_sample(sample_splendid, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 100)
			play_sample(sample_fantastic, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 70)
			play_sample(sample_extreme, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 50)
			play_sample(sample_amazing, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 35)
			play_sample(sample_wow, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 25)
			play_sample(sample_super, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 15)
			play_sample(sample_great, volume_sfx / 10.0);
		else if (it_state.combo_floor >= 7)
			play_sample(sample_sweet, volume_sfx / 10.0);
		else
			play_sample(sample_good, volume_sfx / 10.0);
	}

	switch (it_state.status) {
	case STATUS_IDLE:
		if (it_state.dx > 0.045) {
			if (animation != ANIMATION_WALK_RIGHT) {
				animation = ANIMATION_WALK_RIGHT;
				animation_frame = 0;
			}
		} else if (it_state.dx < -0.045) {
			if (animation != ANIMATION_WALK_LEFT) {
				animation = ANIMATION_WALK_LEFT;
				animation_frame = 0;
			}
		} else if (it_state.y >= 406 && it_state.speed > 0) {
			if (animation != ANIMATION_CHOCK) {
				animation = ANIMATION_CHOCK;
				animation_frame = 0;
			}
		} else {
			if (animation != ANIMATION_IDLE) {
				animation = ANIMATION_IDLE;
				animation_frame = 0;
			}
		}
		if (prev_status != STATUS_IDLE)
			play_sample(sample_step, volume_sfx / 10.0);
		break;
	case STATUS_FLY_UP:
		if (prev_status == STATUS_IDLE) {
			if (it_state.dy < -22.2)
				play_sample(characters[character_index].sfx.jumphi, volume_sfx / 10.0);
			else if (it_state.dy < -15.6)
				play_sample(characters[character_index].sfx.jumpmed, volume_sfx / 10.0);
			else
				play_sample(characters[character_index].sfx.jumplo, volume_sfx / 10.0);
		}
	case STATUS_FLY_IDLE:
	case STATUS_FLY_DOWN:
		if (animation == ANIMATION_ROTATE)
			break;
		if (it_state.dy < -22.2) {
			animation = ANIMATION_ROTATE;
			animation_frame = 0;
		} else if (it_state.dx > 0.045) {
			if (animation != ANIMATION_FLY_RIGHT) {
				animation = ANIMATION_FLY_RIGHT;
				animation_frame = 0;
			}
		} else if (it_state.dx < -0.045) {
			if (animation != ANIMATION_FLY_LEFT) {
				animation = ANIMATION_FLY_LEFT;
				animation_frame = 0;
			}
		} else {
			if (animation != ANIMATION_FLY &&
					animation != ANIMATION_FLY_LEFT &&
					animation != ANIMATION_FLY_RIGHT) {
				animation = ANIMATION_FLY;
				animation_frame = 0;
			}
		}
		break;
	}
	++animation_frame;
}

void draw_background(void) {
	screen_clear(COLOR_BLACK);
}

void draw_floor(FLOOR *floor, unsigned int n) {
	int x, y = it_state.screen_y + 426 - 80 * n;
	unsigned int index = start_floor + n / 100;
	const struct floor_type *type;
	if (index >= floor_types_count)
		index = floor_types_count - 1;
	type = &floor_types[index];
	x = (floor->start + 1) * 16;
	draw_image(type->left, x - (type->left)->w, y);
	while (x < floor->end * 16) {
		draw_image(type->mid, x, y);
		x += 16;
	}
	draw_image(type->right, x, y);
	if (n > 0 && n % 10 == 0) {
		x = (floor->start + floor->end) * 8;
		y += 16;
		draw_image(type->sign, x, y);
		x += (type->sign)->w / 2;
		y += 6;
		draw_textf(font_native, COLOR_BLACK, x - 1, y, ALIGN_CENTER,  "%u", n);
		draw_textf(font_native, COLOR_BLACK, x + 1, y, ALIGN_CENTER,  "%u", n);
		draw_textf(font_native, COLOR_BLACK, x, y - 1, ALIGN_CENTER,  "%u", n);
		draw_textf(font_native, COLOR_BLACK, x, y + 1, ALIGN_CENTER,  "%u", n);
		draw_textf(font_native, COLOR_WHITE, x, y, ALIGN_CENTER,  "%u", n);
	}
}

void draw_floors(void) {
	unsigned int i;
	FLOORS *floors = &it_state.floors;
	for (i = 1; i <= floors->count; ++i) {
		if (i > 7)
			break;
		draw_floor(&floors->floor[(floors->start - i + 7) % 7],
				floors->count - i);
	}
}

void draw_character(void) {
	Image *character = NULL;
	int width, height;
	switch (animation) {
	case ANIMATION_IDLE:
		switch ((animation_frame / 13) % 4) {
		case 0:
		case 2:
			character = characters[character_index].gfx.idle1;
			break;
		case 1:
			character = characters[character_index].gfx.idle2;
			break;
		case 3:
			character = characters[character_index].gfx.idle3;
			break;
		}
		width = (character)->w;
		height = (character)->h;
		draw_image(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_CHOCK:
		character = characters[character_index].gfx.chock;
		width = (character)->w;
		height = (character)->h;
		draw_image(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_WALK_LEFT:
		switch ((animation_frame / 10) % 4) {
		case 0:
			character = characters[character_index].gfx.walk1;
			break;
		case 1:
			character = characters[character_index].gfx.walk2;
			break;
		case 2:
			character = characters[character_index].gfx.walk3;
			break;
		case 3:
			character = characters[character_index].gfx.walk4;
			break;
		}
		width = (character)->w;
		height = (character)->h;
		draw_image_flipped(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_WALK_RIGHT:
		switch ((animation_frame / 10) % 4) {
		case 0:
			character = characters[character_index].gfx.walk1;
			break;
		case 1:
			character = characters[character_index].gfx.walk2;
			break;
		case 2:
			character = characters[character_index].gfx.walk3;
			break;
		case 3:
			character = characters[character_index].gfx.walk4;
			break;
		}
		width = (character)->w;
		height = (character)->h;
		draw_image(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_FLY:
		character = characters[character_index].gfx.jump;
		width = (character)->w;
		height = (character)->h;
		draw_image(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_FLY_LEFT:
		switch (it_state.status) {
		case STATUS_FLY_UP:
			character = characters[character_index].gfx.jump1;
			break;
		case STATUS_FLY_IDLE:
			character = characters[character_index].gfx.jump2;
			break;
		case STATUS_FLY_DOWN:
			character = characters[character_index].gfx.jump3;
			break;
		}
		width = (character)->w;
		height = (character)->h;
		draw_image_flipped(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_FLY_RIGHT:
		switch (it_state.status) {
		case STATUS_FLY_UP:
			character = characters[character_index].gfx.jump1;
			break;
		case STATUS_FLY_IDLE:
			character = characters[character_index].gfx.jump2;
			break;
		case STATUS_FLY_DOWN:
			character = characters[character_index].gfx.jump3;
			break;
		}
		width = (character)->w;
		height = (character)->h;
		draw_image(character, it_state.x - width / 2 + 1, it_state.y - height + 1);
		break;
	case ANIMATION_ROTATE:
		character = characters[character_index].gfx.rotate;
		width = (character)->w;
		height = (character)->h;
		draw_image_pivot(character, width / 2, height / 2, it_state.x, it_state.y - height / 2, M_PI * (animation_frame % 30) / 15.0);
		break;
	}
}

void draw_walls(void) {
	int y = -((80 - it_state.screen_y % 80) % 80) * 1.55;
	for (; y < 480; y += 124) {
		draw_image(bitmap_sideblock, 640 - 75, y);
		draw_image_flipped(bitmap_sideblock, 75 - (bitmap_sideblock)->w, y);
	}
}

void draw_game(void) {
	if (fullscreen)
		screen_clear(RGB(20, 20, 20));
	draw_background();
	draw_floors();
	draw_character();
	draw_walls();
}

void draw_grid(void) {
}

void draw_pause(void) {
	draw_game();
	draw_grid();
	draw_text(font_color, COLOR_WHITE, 190, 160, ALIGN_LEFT,  "GAME PAUSED");
	draw_text(font_mono, COLOR_WHITE, 132, 210, ALIGN_LEFT,  "PRESS ANY KEY TO RESUME");
}

void draw_escape(void) {
	draw_game();
	draw_grid();
	draw_text(font_color, COLOR_WHITE, 23, 160, ALIGN_LEFT,  "DO YOU REALLY WANT TO EXIT?");
	draw_text(font_mono, COLOR_WHITE, 132, 210, ALIGN_LEFT,  "PRESS ANY KEY TO RESUME");
	draw_text(font_mono, COLOR_WHITE, 190, 240, ALIGN_LEFT,  "PRESS ESC TO EXIT");
}

void draw_bottom_hud(void) {
	static unsigned int bottom_combo_timeout = 0;
	draw_image(bitmap_clock, 20, 30);
	draw_image_pivot(bitmap_clock_hand, 8, 29, 56, 77, M_PI * it_state.speed_counter / 750.0);
	draw_textf(font_color, COLOR_WHITE, 24, 420, ALIGN_LEFT,  "SCORE: %d",
			it_state.floor * 10 + it_state.score);
	draw_image(bitmap_combo_meter, 36, 150);
	if (it_state.combo_timer > 0) {
		draw_image_region(bitmap_combo_liquid, 0, 100 - it_state.combo_timer, 16, it_state.combo_timer, 47, 269 - it_state.combo_timer);
		draw_image(bitmap_combo_count, 6, 260);
		draw_textf(font_color, COLOR_WHITE, 56, 264, ALIGN_CENTER, 
				"%u", it_state.combo_floor);
		bottom_combo_timeout = 0;
	} else if (it_state.combo_count > 1) {
		if (bottom_combo_timeout++ < 75) {
		draw_image(bitmap_combo_count, 6, 260);
		draw_textf(font_color, COLOR_WHITE, 56, 264, ALIGN_CENTER, 
				"%u", it_state.combo_floor);
		}
	}
}

void draw_gameover(void) {
	draw_game();
	draw_image(bitmap_gameover, 96, 175);
	draw_text(font_color, COLOR_WHITE, 140, 275, ALIGN_LEFT,  "SCORE:");
	draw_textf(font_color, COLOR_WHITE, 500, 275, ALIGN_RIGHT, 
			"%u", it_state.floor * 10 + it_state.score);
	draw_text(font_color, COLOR_WHITE, 140, 315, ALIGN_LEFT,  "LEVEL:");
	draw_textf(font_color, COLOR_WHITE, 500, 315, ALIGN_RIGHT, 
			"%u", it_state.floor);
	draw_text(font_color, COLOR_WHITE, 140, 355, ALIGN_LEFT,  "BEST COMBO:");
	draw_textf(font_color, COLOR_WHITE, 500, 355, ALIGN_RIGHT, 
			"%u", it_state.combo);
}
