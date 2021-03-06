#pragma once

#include <stdint.h> // int32_t

struct viewport_size {
    int32_t width;
    int32_t height;
};

struct viewport_clip {
    struct viewport_size area;
    int32_t x;
    int32_t y;
};

struct viewport {
    struct viewport_size framebuffer;
    struct viewport_size resolution;
    struct viewport_size offset;
};

enum viewport_mode {
    VIEWPORT_MODE_NORMAL,
    VIEWPORT_MODE_LETTERBOX,
    VIEWPORT_MODE_PILLARBOX
};

void viewport_clip(struct viewport, struct viewport_clip *);
void viewport_box(struct viewport, struct viewport *);

inline
void
viewport_pixel_size(struct viewport const viewport,
                    float * const horizontal,
                    float * const vertical)
{
    *horizontal = (float)viewport.framebuffer.width / viewport.resolution.width;
    *vertical = (float)viewport.framebuffer.height / viewport.resolution.height;
}
