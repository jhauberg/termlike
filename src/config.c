#include <termlike/config.h> // defaults, term_settings
#include <termlike/platform/window.h> // window_size, window_params

#include <stddef.h> // NULL
#include <stdbool.h> // true, false

#include "internal.h" // term_get_*

struct term_settings
defaults(char const * const title)
{
    return (struct term_settings) {
        .title = title,
        .size = (struct term_dimens) {
            .width = 320,
            .height = 240
        },
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
    params->hide_cursor = false;

    params->display.width = resolution.width * params->pixel_size;
    params->display.height = resolution.height * params->pixel_size;
}
