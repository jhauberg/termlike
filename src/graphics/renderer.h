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
};

struct graphics_context;
struct viewport;

struct graphics_context * graphics_init(struct viewport);

void graphics_release(struct graphics_context *);

void graphics_begin(struct graphics_context const *);
void graphics_end(struct graphics_context const *);

// todo: so, graphics responsibilities:
//  * load font texture
//  * draw part of texture
//    oh... but that is actually going to be the spritebatcher..
//    is that assumed part of the graphics impl or can we separate it?
//    i guess not, a Metal implementation would probably do it differently..?
// actually, i guess whether it's a naive or batched approach shouldnt matter
// for the consumer- it's an implementation detail!
// so yes, it should be part of graphics api how it is handled!
void graphics_set_font(struct graphics_context *,
                       struct graphics_image,
                       struct graphics_font);

void graphics_invalidate(struct graphics_context *, struct viewport);

struct viewport graphics_get_viewport(struct graphics_context const *);