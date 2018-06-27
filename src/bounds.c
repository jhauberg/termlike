#include <termlike/bounds.h> // term_bounds

struct term_bounds const TERM_BOUNDS_NONE = {
    .align = TERM_ALIGN_LEFT,
    .wrap = TERM_WRAP_CHARACTERS,
    .width = TERM_BOUNDS_UNBOUNDED,
    .height = TERM_BOUNDS_UNBOUNDED
};

struct term_bounds
bounded(int32_t width, int32_t height)
{
    return (struct term_bounds) {
        .align = TERM_ALIGN_LEFT,
        .wrap = TERM_WRAP_WORDS,
        .width = width,
        .height = height
    };
}

struct term_bounds
aligned(enum term_align const alignment)
{
    struct term_bounds bounds = bounded(TERM_BOUNDS_UNBOUNDED,
                                        TERM_BOUNDS_UNBOUNDED);
    
    return align(bounds, alignment);
}

struct term_bounds
align(struct term_bounds bounds, enum term_align const align)
{
    bounds.align = align;
    
    return bounds;
}
