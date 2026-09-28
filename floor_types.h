#ifndef FLOOR_TYPES_3DS_H
#define FLOOR_TYPES_3DS_H

#include <stddef.h>

#include "gfx.h"

struct floor_type {
    Image *left;
    Image *mid;
    Image *right;
    Image *sign;
};

extern const struct floor_type *floor_types;
extern size_t floor_types_count;

void initialize_floor_types(void);

#endif
