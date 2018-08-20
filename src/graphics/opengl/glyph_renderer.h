#pragma once

#include <gl3w/GL/gl3w.h> // GLuint

#include "renderable.h" // glyph_vertex :completeness

#define GLYPH_VERTEX_COUNT (2 * 3) // 2 triangles per quad = 6 vertices

struct glyph_renderer;
struct viewport;

struct glyph_transform {
    struct vector3 origin;
    struct vector2 offset;
    struct vector2 scale;
    float angle;
};

struct glyph_renderer * glyphs_init(struct viewport);
void glyphs_release(struct glyph_renderer *);

void glyphs_add(struct glyph_renderer *,
                struct glyph_vertex (* vertices)[GLYPH_VERTEX_COUNT],
                struct glyph_transform,
                GLuint texture_id);

void glyphs_invalidate(struct glyph_renderer *, struct viewport);

void glyphs_begin(struct glyph_renderer const *);
void glyphs_end(struct glyph_renderer *);
