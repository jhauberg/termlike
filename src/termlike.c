#include <termlike/termlike.h> // term_*

#include <stdlib.h> // NULL
#include <stdbool.h> // bool

#include "window.h" // window_size, window_params, window_*

static void term_get_window_size(enum term_size, struct window_size *);

static struct window_context * window = NULL;

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
    
    if (window != NULL) {
        window_terminate(window);
    }
    
    window = window_create(params);
    
    if (!window) {
        return false;
    }
    
    return true;
}

bool
term_close(void)
{
    window_terminate(window);
    
    return true;
}

bool
term_should_close(void)
{
    return window == NULL || window_is_closed(window);
}

void
term_render(void)
{
    window_present(window);
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
