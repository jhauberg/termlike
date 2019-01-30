#pragma once

#include "position.h" // term_location :completeness

/**
 * Represents a keyboard input.
 */
enum term_key {
    TERM_KEY_UP,
    TERM_KEY_DOWN,
    TERM_KEY_LEFT,
    TERM_KEY_RIGHT,
    TERM_KEY_CONFIRM,
    TERM_KEY_ESCAPE,
    TERM_KEY_SPACE,
    TERM_KEY_MOUSE_LEFT,
    TERM_KEY_MOUSE_RIGHT,
    /**
     * Any key input.
     *
     * This "key" is triggered when any other `term_key` input is triggered.
     */
    TERM_KEY_ANY
};

/**
 * Represents a mouse/cursor scroll state.
 */
struct term_cursor_scroll {
    /**
     * The horizontal scroll weight.
     *
     * This value is accumulating.
     */
    double horizontal;
    /**
     * The vertical scroll weight.
     *
     * This value is accumulating.
     */
    double vertical;
};

/**
 * Represents a mouse/cursor state.
 */
struct term_cursor_state {
    struct term_cursor_scroll scroll;
    struct term_location location;
};
