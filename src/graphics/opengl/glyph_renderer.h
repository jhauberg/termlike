#pragma once

#include <stdbool.h> // bool

#include <gl3w/GL/gl3w.h> // GLuint

#include "renderable.h" // glyph_vertex :completeness

#define GLYPH_VERTEX_COUNT (2 * 3) // 2 triangles per quad = 6 vertices

struct glyph_renderer;
struct viewport;

struct glyph_transform {
    struct vector3 origin;
    struct vector2 offset;
    float angle;
    float horizontal_scale;
    float vertical_scale;
};

struct glyph_renderer * glyphs_init(struct viewport);
void glyphs_release(struct glyph_renderer *);

void glyphs_add(struct glyph_renderer *,
                struct glyph_vertex (* vertices)[GLYPH_VERTEX_COUNT],
                struct glyph_transform,
                GLuint texture_id);

void glyphs_invalidate(struct glyph_renderer *, struct viewport);

bool glyphs_flush(struct glyph_renderer *);
void glyphs_reset(struct glyph_renderer *);
