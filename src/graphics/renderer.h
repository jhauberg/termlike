#pragma once

#include <stdint.h> // uint8_t, int32_t

struct graphics_image {
    int32_t width;
    int32_t height;
    int32_t components;
    uint8_t * data;
};

struct graphics_font {
    uint32_t const * codepage;
    uint16_t columns;
    uint16_t rows;
    uint16_t size;
};

struct graphics_color {
    float r, g, b, a;
};

struct graphics_position {
    float z;
    int32_t x;
    int32_t y;
};

struct graphics_transform {
    struct graphics_position position;
    float horizontal_scale;
    float vertical_scale;
    float angle;
};

struct graphics_context;
struct viewport;

struct graphics_context * graphics_init(struct viewport);

void graphics_release(struct graphics_context *);

void graphics_begin(struct graphics_context *);
void graphics_end(struct graphics_context *);

void graphics_draw(struct graphics_context const *,
                   struct graphics_color,
                   struct graphics_transform,
                   uint32_t code);

struct graphics_font graphics_get_font(struct graphics_context const *);
void graphics_set_font(struct graphics_context *,
                       struct graphics_image,
                       struct graphics_font);

void graphics_invalidate(struct graphics_context *, struct viewport);

struct viewport graphics_get_viewport(struct graphics_context const *);
