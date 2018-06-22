#pragma once

#include <termlike/position.h> // term_location :completeness

#include <stdint.h> // int32_t

struct cursor {
    struct term_location offset;
    int32_t width;
    int32_t height;
};

struct term_bounds;

void cursor_start(struct cursor *,
                  int32_t width,
                  int32_t height);

void cursor_advance(struct cursor *,
                    uint32_t character,
                    struct term_bounds);
