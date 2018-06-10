#pragma once

// todo: these are not actually keys, per se, they're
//       more like actions mapped to one or more keys

enum term_key {
    TERM_KEY_UP,
    TERM_KEY_DOWN,
    TERM_KEY_LEFT,
    TERM_KEY_RIGHT,
    TERM_KEY_CONFIRM,
    TERM_KEY_ESCAPE,
    TERM_KEY_SPACE,
    TERM_KEY_ANY
};

#define TERM_KEY_FIRST (TERM_KEY_UP)
#define TERM_KEY_MAX (TERM_KEY_ANY + 1)
