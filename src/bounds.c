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
    return boxed(width, height, TERM_ALIGN_LEFT);
}

struct term_bounds
aligned(enum term_align const alignment)
{
    return boxed(TERM_BOUNDS_UNBOUNDED, TERM_BOUNDS_UNBOUNDED, alignment);
}

struct term_bounds
boxed(int32_t const width, int32_t const height, enum term_align const align)
{
    return (struct term_bounds) {
        .align = align,
        .wrap = TERM_WRAP_WORDS,
        .width = width,
        .height = height
    };
}
