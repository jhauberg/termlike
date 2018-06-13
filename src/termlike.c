#include <termlike/termlike.h> // term_*
#include <termlike/config.h> // term_settings, term_size
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdbool.h> // bool

#include "keys.h" // term_key_state

#include "platform/window.h" // window_size, window_params, window_*
#include "platform/timer.h" // timer, timer_*

#include "graphics/renderer.h" // graphics_context, graphics_*
#include "graphics/viewport.h" // viewport
#include "graphics/loader.h" // load_image_data

#include "resources/spritefont.8x8.h" // IBM8x8*

struct term_context {
    term_draw_callback * draw_func;
    term_tick_callback * tick_func;
    struct window_context * window;
    struct graphics_context * graphics;
    struct timer * timer;
    struct term_key_state keys;
    struct term_cursor_state cursor;
};

static bool term_setup(struct window_size);
static void term_invalidate(void);

static void term_handle_internal_input(void);

static void term_toggle_fullscreen(void);

static void term_get_display_size(enum term_size, struct window_size *);
static void term_get_display_params(struct term_settings,
                                    struct window_size,
                                    struct window_params *);

static void term_callback_font_loaded(struct graphics_image);

static struct term_context terminal;

bool
term_open(struct term_settings const settings)
{
    if (terminal.window != NULL) {
        window_terminate(terminal.window);
    }
    
    struct window_size display;
    struct window_params params;
    
    term_get_display_size(settings.size, &display);
    term_get_display_params(settings, display, &params);
    
    if (params.display.width == 0 ||
        params.display.height == 0) {
        return false;
    }
    
    terminal.window = window_create(params);
    
    if (terminal.window == NULL) {
        return false;
    }

    if (!term_setup(display)) {
        return false;
    }
    
    return true;
}

bool
term_close(void)
{
    graphics_release(terminal.graphics);
    timer_release(terminal.timer);
    
    window_terminate(terminal.window);
 
    terminal.graphics = NULL;
    terminal.timer = NULL;
    terminal.window = NULL;
    
    terminal.draw_func = NULL;
    terminal.tick_func = NULL;
    
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
term_set_drawing(term_draw_callback * const draw_func)
{
    terminal.draw_func = draw_func;
}

void
term_set_ticking(term_tick_callback * const tick_func)
{
    terminal.tick_func = tick_func;
}

void
term_run(uint16_t const frequency)
{
    struct window_context * const window = terminal.window;
    
    window_read(window, &terminal.keys, &terminal.cursor);
    
    term_handle_internal_input();
    
    double step = 0;
    double interpolate = 0;
    
    timer_begin(terminal.timer); {
        while (timer_tick(terminal.timer, frequency, &step)) {
            // update input (again) for every tick this frame
            window_read(window, &terminal.keys, &terminal.cursor);
            
            if (terminal.tick_func) {
                terminal.tick_func(step);
            }
        }
    }
    timer_end(terminal.timer, &interpolate);
    
    graphics_begin(terminal.graphics); {
        if (terminal.draw_func) {
            terminal.draw_func(interpolate);
        }
    }
    graphics_end(terminal.graphics);
    
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
bool
term_setup(struct window_size const display)
{
    struct viewport viewport;
    
    viewport.offset.width = 0;
    viewport.offset.height = 0;
    
    viewport.resolution.width = display.width;
    viewport.resolution.height = display.height;
    
    terminal.graphics = graphics_init(viewport);
    
    if (terminal.graphics == NULL) {
        return false;
    }
    
    terminal.timer = timer_init();
    
    if (terminal.timer == NULL) {
        return false;
    }
    
    load_image_data(IBM8x8_FONT, IBM8x8_LENGTH, term_callback_font_loaded);
    
    term_invalidate();
    
    return true;
}

static
void
term_callback_font_loaded(struct graphics_image const image)
{
    struct graphics_font font;
    
    font.columns = IBM8x8_COLUMNS;
    font.rows = IBM8x8_ROWS;
    font.codepage = IBM8x8_CODEPAGE;
    
    graphics_set_font(terminal.graphics, image, font);
}

static
void
term_invalidate(void)
{
    struct viewport viewport = graphics_get_viewport(terminal.graphics);
    
    window_get_framebuffer_size(terminal.window,
                                &viewport.framebuffer.width,
                                &viewport.framebuffer.height);
    
    graphics_invalidate(terminal.graphics, viewport);
}

static
void
term_handle_internal_input(void)
{
    if (term_key_released((enum term_key)TERM_KEY_TOGGLE_FULLSCREEN)) {
        term_toggle_fullscreen();
    }
}

static
void
term_toggle_fullscreen(void)
{
    struct window_context * const window = terminal.window;
    
    window_set_fullscreen(window, !window_is_fullscreen(window));
    
    // note that generally, the graphics context should be invalidated
    // whenever the window is resized in any way- however, since the window
    // is currently limited to the initial size, resizing *only* happens
    // when switching between fullscreen/windowed
    term_invalidate();
}

static
void
term_get_display_params(struct term_settings const settings,
                        struct window_size const resolution,
                        struct window_params * const params)
{
    params->title = settings.title != NULL ? settings.title : "";
    params->pixel_size = settings.pixel_size > 0 ? settings.pixel_size : 1;
    params->swap_interval = settings.vsync ? 1 : 0;
    params->fullscreen = settings.fullscreen;
    
    params->display.width = resolution.width * params->pixel_size;
    params->display.height = resolution.height * params->pixel_size;
}

static
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
