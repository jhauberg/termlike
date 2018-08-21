#pragma once

#include <termlike/bounds.h> // term_bounds :completeness

#include <stdint.h> // uint32_t
#include <stdbool.h> // bool

struct cursor_offset {
    // note that the cursor must operate on transformed glyph sizes;
    // i.e. not just logical fixed-point advancements, because the provided
    // bounds are always in absolute dimensions
    float x, y;
    uint32_t line;
};

struct cursor {
    struct term_bounds bounds;
    struct cursor_offset offset;
    float width;
    float height;
};

void cursor_start(struct cursor *,
                  struct term_bounds,
                  float width,
                  float height);

void cursor_advance(struct cursor *,
                    struct cursor_offset *,
                    uint32_t character);

bool cursor_is_out_of_bounds(struct cursor const *);
