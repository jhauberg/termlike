#pragma once

#include <stdint.h> // uint8_t, int32_t

struct term_color {
    float a;
    uint8_t r, g, b;
};

extern struct term_color const TERM_COLOR_WHITE;
extern struct term_color const TERM_COLOR_BLACK;

struct term_color coloredh(int32_t hex);

/**
 * Return an opaque color.
 */
struct term_color colored(uint8_t red,
                          uint8_t green,
                          uint8_t blue);
/**
 * Return a transparent color.
 */
struct term_color transparent(struct term_color,
                              float alpha);
