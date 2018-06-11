#pragma once

#include <termlike/input.h> // TERM_KEY_MAX

#include <stdbool.h> // bool

struct term_key_state {
    bool down[TERM_KEY_MAX];
    bool pressed[TERM_KEY_MAX];
    bool released[TERM_KEY_MAX];
};
