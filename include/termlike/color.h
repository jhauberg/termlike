#pragma once

#include <stdint.h> // uint8_t, int32_t

/**
 * Represents a color.
 *
 * Colors are specified in RGBA, values ranging from 0-255 per component.
 */
struct term_color {
    uint8_t r, g, b, a;
};

/**
 * The color white.
 */
extern struct term_color const TERM_COLOR_WHITE;
/**
 * The color black.
 */
extern struct term_color const TERM_COLOR_BLACK;

/**
 * Alpha value for an opaque color.
 */
#define TERM_COLOR_OPAQUE (255)
/**
 * Alpha value for a fully transparent color.
 */
#define TERM_COLOR_TRANSPARENT (0)

/**
 * Return the (opaque) color corresponding to RGB-components.
 *
 * For example, [255, 0, 0] for a white color.
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
        .a = TERM_COLOR_OPAQUE
    };
}

/**
 * Return the (opaque) color corresponding to a hex value.
 *
 * For example, `0xFF0000` for a white color.
 */
inline
struct term_color
coloredh(int32_t const hex)
{
    return colored((uint8_t)((hex & 0xFF0000) >> 16),
                   (uint8_t)((hex & 0xFF00) >> 8),
                   (uint8_t)((hex & 0xFF) >> 0));
}

/**
 * Return the color corresponding to a color with an alpha value.
 */
inline
struct term_color
transparent(struct term_color color, uint8_t const alpha)
{
    color.a = alpha;
    
    return color;
}
