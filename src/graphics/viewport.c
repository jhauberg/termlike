#include "viewport.h" // viewport, viewport_*

#include <stdint.h> // int32_t
#include <math.h> // roundf

static void viewport_get_mode(struct viewport,
                              struct viewport_size *,
                              enum viewport_mode *);

extern inline void viewport_pixel_size(struct viewport,
                                       float * horizontal,
                                       float * vertical);

void
viewport_clip(struct viewport const viewport,
              struct viewport_clip * const clip)
{
    *clip = (struct viewport_clip) {
        .area = {
            .width = viewport.framebuffer.width - viewport.offset.width,
            .height = viewport.framebuffer.height - viewport.offset.height
        },
        .x = viewport.offset.width / 2,
        .y = viewport.offset.height / 2
    };
}

void
viewport_box(struct viewport viewport,
             struct viewport * const box)
{
    *box = viewport;
    
    box->offset.width = 0;
    box->offset.height = 0;
    
    struct viewport_size target;
    enum viewport_mode mode;
    
    viewport_get_mode(viewport, &target, &mode);
    
    struct viewport_size framebuffer = viewport.framebuffer;
    
    if (mode != VIEWPORT_MODE_NORMAL) {
        box->offset.width = framebuffer.width - target.width;
        box->offset.height = framebuffer.height - target.height;
    }
}

static
void
viewport_get_mode(struct viewport const viewport,
                  struct viewport_size * const target,
                  enum viewport_mode * const mode)
{
    float const screen_aspect_ratio =
        (float)viewport.resolution.width / (float)viewport.resolution.height;
    float const framebuffer_aspect_ratio =
        (float)viewport.framebuffer.width / (float)viewport.framebuffer.height;
    
    float adjusted_width = viewport.framebuffer.width;
    float adjusted_height = viewport.framebuffer.height;
    
    *mode = VIEWPORT_MODE_NORMAL;
    
    if (screen_aspect_ratio > framebuffer_aspect_ratio ||
        framebuffer_aspect_ratio > screen_aspect_ratio) {
        float const targetAspectRatio = screen_aspect_ratio;
        
        // letterbox (horizontal bars)
        adjusted_height = roundf(adjusted_width / targetAspectRatio);
        
        *mode = VIEWPORT_MODE_LETTERBOX;
        
        if (adjusted_height > viewport.framebuffer.height) {
            // pillarbox (vertical bars)
            adjusted_height = viewport.framebuffer.height;
            adjusted_width = roundf(adjusted_height * targetAspectRatio);
            
            *mode = VIEWPORT_MODE_PILLARBOX;
        }
    }
    
    target->width = (int32_t)adjusted_width;
    target->height = (int32_t)adjusted_height;
}

