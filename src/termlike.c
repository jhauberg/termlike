#include <termlike/termlike.h> // term_*
#include <termlike/config.h> // term_settings, term_size
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdbool.h> // bool

#include "state.h" // term_key_state
#include "window.h" // window_size, window_params, window_*

struct term_context {
    struct window_context * window;
    struct term_key_state keys;
    struct term_cursor_state cursor;
};

static void term_get_window_size(enum term_size, struct window_size *);

static struct term_context terminal;

bool
term_open(struct term_settings const settings)
{
    if (terminal.window != NULL) {
        window_terminate(terminal.window);
    }
    
    struct window_size native = { 0, 0 };
    
    term_get_window_size(settings.size, &native);
    
    struct window_params params;
    
    params.title = settings.title != NULL ? settings.title : "";
    params.pixel_size = settings.pixel_size > 0 ? settings.pixel_size : 1;
    params.swap_interval = settings.vsync ? 1 : 0;
    params.fullscreen = settings.fullscreen;
    
    params.display = (struct window_size) {
        .width = native.width * params.pixel_size,
        .height = native.height * params.pixel_size
    };
    
    if (params.display.width == 0 ||
        params.display.height == 0) {
        return false;
    }
    
    terminal.window = window_create(params);
    
    if (terminal.window == NULL) {
        return false;
    }
    
    return true;
}

bool
term_close(void)
{
    window_terminate(terminal.window);
    
    return true;
}

bool
term_is_closing(void)
{
    return terminal.window == NULL || window_is_closed(terminal.window);
}

void
term_set_closing(bool const close)
{
    window_set_closed(terminal.window, close);
}

void
term_render(void)
{
    struct window_context * const window = terminal.window;
    
    window_read(window, &terminal.keys, &terminal.cursor);

    if (term_key_released(TERM_KEY_TOGGLE_FULLSCREEN)) {
        window_set_fullscreen(window, !window_is_fullscreen(window));
    }

    window_present(window);
}

bool
term_key_down(enum term_key const key)
{
    return terminal.keys.down[key];
}

bool
term_key_pressed(enum term_key const key)
{
    return terminal.keys.pressed[key];
}

bool
term_key_released(enum term_key const key)
{
    return terminal.keys.released[key];
}

struct term_cursor_state
term_cursor(void)
{
    return (struct term_cursor_state) {
        .location = terminal.cursor.location,
        .scroll = terminal.cursor.scroll
    };
}

static
void
term_get_window_size(enum term_size size, struct window_size * const display)
{
    switch (size) {
        case TERM_SIZE_320: {
            display->width = 320;
            display->height = 240;
        } break;
            
        default:
            break;
    }
}
