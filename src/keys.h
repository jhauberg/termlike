#pragma once

#include <stdbool.h> // bool

#define TERM_KEY_FIRST (TERM_KEY_UP)
#define TERM_KEY_LAST (TERM_KEY_ANY)

// extend key range with an internal set of actions
// (note that these can be safely typecast to `enum term_key`)
enum term_key_internal {
    TERM_KEY_TOGGLE_FULLSCREEN = TERM_KEY_LAST + 1,
    // note that we're leaving profiling toggle in, even for release builds
    // (makes it easier to keep TERM_KEY_MAX correct in both cases)
    TERM_KEY_TOGGLE_PROFILING
};

#define TERM_KEY_INTERNAL_LAST (TERM_KEY_TOGGLE_PROFILING)

#define TERM_KEY_MAX (TERM_KEY_INTERNAL_LAST + 1)

struct term_key_state {
    bool down[TERM_KEY_MAX];
    bool pressed[TERM_KEY_MAX];
    bool released[TERM_KEY_MAX];
};
