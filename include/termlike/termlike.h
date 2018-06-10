#pragma once

#include <termlike/input.h> // term_key :completeness

#include <stdint.h> // uint8_t
#include <stdbool.h> // bool

enum term_size {
    TERM_SIZE_320
};

struct term_settings {
    /** Text shown in the title bar of the window. */
    char const * title;
    /** Size of the display within the window, in pixels.
     *
     * Actual window size is determined by pixel-size and may be larger
     * than the display. */
    enum term_size size;
    /** Size of a pixel.
     *
     * Default is 1. A higher pixel size results in a larger window. */
    uint8_t pixel_size;
    /** Determines whether the window should open as fullscreen initially. */
    bool fullscreen;
    /** Determines whether v-sync should be enabled. */
    bool vsync;
};

struct term_settings term_settings(char const * title);

bool term_open(struct term_settings);
bool term_close(void);

bool term_is_closing(void);
void term_set_closing(bool close);

void term_render(void);

bool term_key_down(enum term_key);
bool term_key_pressed(enum term_key);
bool term_key_released(enum term_key);
