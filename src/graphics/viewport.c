#include "viewport.h" // viewport, viewport_*

#include <stdint.h> // int32_t
#include <math.h> // roundf

static enum viewport_mode viewport_get_mode(struct viewport,
                                            int32_t * width,
                                            int32_t * height);

struct viewport_clip
viewport_get_clip(struct viewport const viewport)
{
    return (struct viewport_clip) {
        .area = {
            .width = viewport.framebuffer.width - viewport.offset.width,
            .height = viewport.framebuffer.height - viewport.offset.height
        },
        .x = viewport.offset.width / 2,
        .y = viewport.offset.height / 2
    };
}

struct viewport
viewport_box(struct viewport viewport)
{
    viewport.offset.width = 0;
    viewport.offset.height = 0;
    
    struct viewport_size target;
    
    enum viewport_mode const mode = viewport_get_mode(viewport,
                                                      &target.width,
                                                      &target.height);
    
    struct viewport_size framebuffer = viewport.framebuffer;
    
    if (mode != VIEWPORT_MODE_NORMAL) {
        viewport.offset.width = framebuffer.width - target.width;
        viewport.offset.height = framebuffer.height - target.height;
    }
    
    return viewport;
}

double
viewport_pixel(struct viewport const viewport)
{
    return (viewport.framebuffer.width + viewport.framebuffer.height) /
        (viewport.resolution.width + viewport.resolution.height);
}

static
enum viewport_mode
viewport_get_mode(struct viewport const viewport,
                  int32_t * const width,
                  int32_t * const height)
{
    float const screen_aspect_ratio =
        (float)viewport.resolution.width / (float)viewport.resolution.height;
    float const framebuffer_aspect_ratio =
        (float)viewport.framebuffer.width / (float)viewport.framebuffer.height;
    
    float adjusted_width = viewport.framebuffer.width;
    float adjusted_height = viewport.framebuffer.height;
    
    enum viewport_mode mode = VIEWPORT_MODE_NORMAL;
    
    if (screen_aspect_ratio > framebuffer_aspect_ratio ||
        framebuffer_aspect_ratio > screen_aspect_ratio) {
        float const targetAspectRatio = screen_aspect_ratio;
        
        // letterbox (horizontal bars)
        adjusted_height = roundf(adjusted_width / targetAspectRatio);
        
        mode = VIEWPORT_MODE_LETTERBOX;
        
        if (adjusted_height > viewport.framebuffer.height) {
            // pillarbox (vertical bars)
            adjusted_height = viewport.framebuffer.height;
            adjusted_width = roundf(adjusted_height * targetAspectRatio);
            
            mode = VIEWPORT_MODE_PILLARBOX;
        }
    }
    
    *width = (int32_t)adjusted_width;
    *height = (int32_t)adjusted_height;
    
    return mode;
}

