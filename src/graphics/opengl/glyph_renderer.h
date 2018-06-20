#pragma once

#include <stdbool.h> // bool

#include <gl3w/GL/gl3w.h> // GLuint

#include "renderable.h" // glyph_vertex :completeness

struct glyph_renderer;
struct viewport;

struct glyph_renderer * glyphs_init(struct viewport);
void glyphs_release(struct glyph_renderer *);

void glyphs_add(struct glyph_renderer *,
                struct glyph_vertex const * vertices,
                struct vector3 origin,
                float angle,
                float scale,
                GLuint texture_id);

void glyphs_invalidate(struct glyph_renderer *, struct viewport);

bool glyphs_flush(struct glyph_renderer *);
void glyphs_reset(struct glyph_renderer *);
