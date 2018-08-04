#pragma once

#include <termlike/position.h> // term_location :completeness

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

struct cursor {
    float x, y;
    float width;
    float height;
    int32_t breaks;
};

struct term_bounds;

void cursor_start(struct cursor *,
                  float width,
                  float height);

void cursor_advance(struct cursor *,
                    struct term_bounds,
                    uint32_t character);

bool cursor_is_out_of_bounds(struct cursor const *, struct term_bounds);
