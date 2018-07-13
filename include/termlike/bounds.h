#pragma once

#include <stdint.h> // int32_t

enum term_wrap {
    TERM_WRAP_CHARACTERS,
    TERM_WRAP_WORDS
};

enum term_align {
    TERM_ALIGN_LEFT,
    TERM_ALIGN_RIGHT,
    TERM_ALIGN_CENTER
};

struct term_dimens {
    int32_t width;
    int32_t height;
};

struct term_bounds {
    enum term_wrap wrap;
    enum term_align align;
    struct term_dimens size;
};

#define TERM_BOUNDS_UNBOUNDED (0)

extern struct term_bounds const TERM_BOUNDS_NONE;

struct term_dimens sized(int32_t width, int32_t height);

struct term_bounds bounded(int32_t width, int32_t height);
struct term_bounds aligned(enum term_align);

struct term_bounds boxed(int32_t width, int32_t height, enum term_align);

