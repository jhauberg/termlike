#include <termlike/config.h> // defaults, term_settings, term_size, TERM_SIZE_*

#include <stddef.h> // NULL
#include <stdbool.h> // true, false

#include "platform/window.h" // window_size, window_params

#include "internal.h" // term_get_*

struct term_settings
defaults(char const * const title)
{
    return (struct term_settings) {
        .title = title,
        .size = TERM_SIZE_320,
        .pixel_size = 1,
        .fullscreen = false,
        .vsync = true
    };
}

void
term_get_display_params(struct term_settings const settings,
                        struct window_size const resolution,
                        struct window_params * const params)
{
    params->title = settings.title != NULL ? settings.title : "";
    params->pixel_size = settings.pixel_size > 0 ? settings.pixel_size : 1;
    params->swap_interval = settings.vsync ? 1 : 0;
    params->fullscreen = settings.fullscreen;
    params->hide_cursor = true;
    
    params->display.width = resolution.width * params->pixel_size;
    params->display.height = resolution.height * params->pixel_size;
}

void
term_get_display_size(enum term_size const size,
                      struct window_size * const display)
{
    switch (size) {
        case TERM_SIZE_320: {
            display->width = 320;
            display->height = 240;
        } break;
            
        default: {
            display->width = 0;
            display->height = 0;
        } break;
    }
}
