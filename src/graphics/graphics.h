#pragma once

struct graphics_context;
struct viewport;

struct graphics_context * graphics_init(struct viewport);
void graphics_release(struct graphics_context *);

void graphics_begin(struct graphics_context const *);
void graphics_end(struct graphics_context const *);

void graphics_invalidate(struct graphics_context *, struct viewport);

struct viewport graphics_get_viewport(struct graphics_context const *);
