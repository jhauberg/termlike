#pragma once

#include <stdint.h> // int32_t

/**
 * Represents the text-wrapping mode.
 */
enum term_wrap {
    /**
     * Wrap on characters.
     */
    TERM_WRAP_CHARACTERS,
    /**
     * Wrap on words.
     */
    TERM_WRAP_WORDS
};

/**
 * Represents the alignment of text.
 */
enum term_align {
    TERM_ALIGN_LEFT,
    TERM_ALIGN_RIGHT,
    TERM_ALIGN_CENTER
};

/**
 * Represents a size (in pixels).
 */
struct term_dimens {
    int32_t width;
    int32_t height;
};

/**
 * Represents a bounded area.
 *
 * The bounded area serves as constraints to horizontal and vertical
 * text wrapping.
 */
struct term_bounds {
    struct term_dimens size;
    enum term_wrap wrap;
    enum term_align align;
};

/**
 * Unbounded area with default alignment and wrapping mode.
 */
extern struct term_bounds const TERM_BOUNDS_NONE;

/**
 * No boundary.
 *
 * This can be specified to disable text-wrapping on a specific axis; e.g.
 * only wrapping horizontally, but not vertically, or vice versa.
 */
#define TERM_BOUNDS_UNBOUNDED (0)

inline
struct term_dimens
sized(int32_t const width, int32_t const height)
{
    return (struct term_dimens) {
        .width = width,
        .height = height
    };
}

inline
struct term_bounds
boxed(int32_t const width,
      int32_t const height,
      enum term_align const alignment)
{
    return (struct term_bounds) {
        .align = alignment,
        .wrap = TERM_WRAP_WORDS,
        .size = {
            .width = width,
            .height = height
        }
    };
}
    
inline
struct term_bounds
bounded(int32_t const width, int32_t const height)
{
    return boxed(width, height, TERM_ALIGN_LEFT);
}
    
inline
struct term_bounds
aligned(enum term_align const alignment)
{
    return boxed(TERM_BOUNDS_UNBOUNDED, TERM_BOUNDS_UNBOUNDED, alignment);
}
