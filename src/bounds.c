#include <termlike/bounds.h> // term_bounds

struct term_bounds const TERM_BOUNDS_NONE = {
    .wrap = TERM_WRAP_CHARACTERS,
    .width = TERM_BOUNDS_UNBOUNDED,
    .height = TERM_BOUNDS_UNBOUNDED
};

struct term_bounds
bounded(int32_t const width, int32_t const height)
{
    return boundedw(width, height, TERM_WRAP_WORDS);
}

struct term_bounds
boundedw(int32_t const width, int32_t const height, enum term_wrap const wrap)
{
    return (struct term_bounds) {
        .wrap = wrap,
        .width = width,
        .height = height
    };
}
