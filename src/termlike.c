#include <termlike/termlike.h> // term_*
#include <termlike/input.h> // term_key

#include <stdlib.h> // NULL
#include <stdbool.h> // bool

#include "state.h" // key_state
#include "window.h" // window_size, window_params, window_*

struct term_context {
    struct window_context * window;
    struct key_state input;
};

static void term_get_window_size(enum term_size, struct window_size *);

static struct term_context terminal;

struct term_settings
term_settings(char const * const title)
{
    return (struct term_settings) {
        .title = title,
        .size = TERM_SIZE_320,
        .pixel_size = 1,
        .fullscreen = false,
        .vsync = true
    };
}

bool
term_open(struct term_settings const settings)
{
    struct window_size native;
    
    term_get_window_size(settings.size, &native);
    
    struct window_params params;
    
    params.title = settings.title;
    params.pixel_size = settings.pixel_size > 0 ? settings.pixel_size : 1;
    params.swap_interval = settings.vsync ? 1 : 0;
    params.fullscreen = settings.fullscreen;
    
    params.display = (struct window_size) {
        .width = native.width * params.pixel_size,
        .height = native.height * params.pixel_size
    };
    
    if (terminal.window != NULL) {
        window_terminate(terminal.window);
    }
    
    terminal.window = window_create(params);
    
    if (!terminal.window) {
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
    window_read(terminal.window, &terminal.input);
    // todo: timing
    // todo: drawing
    // todo: all of the above should be handled here-
    //       client should provide callbacks for ticks, render etc.
    window_present(terminal.window);
}

bool
term_key_down(enum term_key const key)
{
    return terminal.input.down[key];
}

bool
term_key_pressed(enum term_key const key)
{
    return terminal.input.pressed[key];
}

bool
term_key_released(enum term_key const key)
{
    return terminal.input.released[key];
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
