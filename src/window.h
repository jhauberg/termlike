#pragma once

#include <stdint.h> // uint8_t, int32_t
#include <stdbool.h> // bool

struct window_size {
    int32_t width;
    int32_t height;
};

struct window_params {
    /** Text shown in the title bar of the window. */
    char const * title;
    /** The display size; e.g. native resolution ⨉ pixel size */
    struct window_size display;
    /** The number of screen updates between buffer swaps */
    uint8_t swap_interval;
    /** The size of each pixel; must be higher than 0 */
    uint8_t pixel_size;
    /** Determines whether the context is running in fullscreen */
    bool fullscreen;
};

struct window_context;

struct term_key_state;
struct term_cursor_state;

struct window_context * window_create(struct window_params);
void window_terminate(struct window_context *);

void window_set_closed(struct window_context *, bool closed);
bool window_is_closed(struct window_context *);

bool window_is_fullscreen(struct window_context const *);
void window_set_fullscreen(struct window_context *, bool fullscreen);

void window_present(struct window_context const *);
void window_read(struct window_context *,
                 struct term_key_state *,
                 struct term_cursor_state *);
