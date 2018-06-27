#pragma once

#include <termlike/position.h> // term_location :completeness

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

struct cursor {
    struct term_location offset;
    int32_t width;
    int32_t height;
    int32_t breaks;
};

struct term_bounds;

void cursor_start(struct cursor *,
                  int32_t width,
                  int32_t height);

void cursor_advance(struct cursor *,
                    struct term_bounds,
                    uint32_t character);

bool cursor_is_out_of_bounds(struct cursor const *, struct term_bounds);
