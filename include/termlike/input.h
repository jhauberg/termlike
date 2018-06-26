#pragma once

#include "position.h" // term_location :completeness

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
    TERM_KEY_ANY
};

struct term_cursor_scroll {
    double horizontal;
    double vertical;
};

struct term_cursor_state {
    struct term_cursor_scroll scroll;
    struct term_location location;
};
