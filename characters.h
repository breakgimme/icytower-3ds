#ifndef CHARACTERS_3DS_H
#define CHARACTERS_3DS_H

#include <stddef.h>

#include "gfx.h"
#include "sfx.h"

struct character_gfx {
    Image *idle1;
    Image *idle2;
    Image *idle3;
    Image *walk1;
    Image *walk2;
    Image *walk3;
    Image *walk4;
    Image *jump1;
    Image *jump2;
    Image *jump3;
    Image *jump;
    Image *chock;
    Image *rotate;
    Image *edge1;
    Image *edge2;
};

struct character_sfx {
    Sound *greeting;
    Sound *jumplo;
    Sound *jumpmed;
    Sound *jumphi;
    Sound *edge;
    Sound *death;
    Sound *pause;
    Music *bgmusic;
};

struct character {
    struct character_gfx gfx;
    struct character_sfx sfx;
};

extern const struct character *characters;
extern size_t characters_count;

void initialize_characters(void);

#endif
