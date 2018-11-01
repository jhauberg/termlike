#pragma once

#include <termlike/bounds.h> // term_dimens :completeness

#include <stdint.h> // uint8_t
#include <stdbool.h> // bool

struct term_settings {
    /** Text shown in the title bar of the window. */
    char const * title;
    /** Size of the display within the window, in pixels.
     *
     * Actual window size is determined by pixel-size and may be larger
     * than the intended resolution. */
    struct term_dimens size;
    /** Size of a pixel.
     *
     * Default is 1. A higher pixel size results in a larger window. */
    uint8_t pixel_size;
    /** Determines whether the window should open as fullscreen initially. */
    bool fullscreen;
    /** Determines whether v-sync should be enabled. */
    bool vsync;
};

struct term_settings defaults(char const * title);
