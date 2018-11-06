#pragma once

#include <stdint.h> // uint8_t, int32_t

struct term_color {
    uint8_t r, g, b, a;
};

extern struct term_color const TERM_COLOR_WHITE;
extern struct term_color const TERM_COLOR_BLACK;

/**
 * Return an opaque color.
 */
inline
struct term_color
colored(uint8_t const red,
        uint8_t const green,
        uint8_t const blue)
{
    return (struct term_color) {
        .r = red,
        .g = green,
        .b = blue,
        .a = 255
    };
}

/**
 * Return an opaque color from a hex value.
 */
inline
struct term_color
coloredh(int32_t const hex)
{
    return colored((hex & 0xFF0000) >> 16,
                   (hex & 0xFF00) >> 8,
                   (hex & 0xFF) >> 0);
}

/**
 * Return a transparent color.
 */
inline
struct term_color
transparent(struct term_color color, uint8_t const alpha)
{
    color.a = alpha;
    
    return color;
}
