#pragma once

#include <stdint.h> // int32_t

enum term_wrap {
    TERM_WRAP_CHARACTERS,
    TERM_WRAP_WORDS
};

struct term_bounds {
    enum term_wrap wrap;
    int32_t width;
    int32_t height;
};

#define TERM_BOUNDS_UNBOUNDED (0)

extern struct term_bounds const TERM_BOUNDS_NONE;

struct term_bounds bounded(int32_t width, int32_t height);
struct term_bounds boundedw(int32_t width, int32_t height, enum term_wrap);
