#include <termlike/termlike.h> // term_*
#include <termlike/layer.h> // term_layer
#include <termlike/config.h> // term_settings, term_size
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdint.h> // uint16_t, uint32_t, int32_t
#include <stdbool.h> // bool

#include "internal.h" // layer_z
#include "buffer.h" // buffer, buffer_offset, buffer_dimens, buffer_*
#include "keys.h" // term_key_state

#include "graphics/renderer.h" // graphics_context, graphics_*
#include "graphics/viewport.h" // viewport
#include "graphics/loader.h" // load_image_data

#include "platform/window.h" // window_size, window_params, window_*
#include "platform/timer.h" // timer, timer_*

#ifdef DEBUG
 #include "platform/profiler.h" // profiler_*
#endif

#include "resources/cp437.h" // CP437
#include "resources/spritefont.8x8.h" // IBM8x8*

/**
 * Provides values for accumulating the required dimensions of
 * printable characters in a buffer.
 */
struct term_state_measure {
    int32_t width;
    int32_t height;
};

/**
 * Provides initial values for positioning and coloring of printable
 * characters in a buffer.
 */
struct term_state_print {
    struct graphics_color tint;
    float z;
    int32_t x;
    int32_t y;
};

struct term_context {
    term_draw_callback * draw_func;
    term_tick_callback * tick_func;
    struct window_context * window;
    struct graphics_context * graphics;
    struct timer * timer;
    struct buffer * buffer;
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

/**
 * Print a character at an offset.
 *
 * This function can be passed to a buffer as a character callback.
 */
static void term_print_character(struct buffer_offset,
                                 struct buffer_dimens,
                                 uint32_t character,
                                 void *);
/**
 * Measure a character at an offset.
 *
 * This function can be passed to a buffer as a character callback.
 */
static void term_measure_character(struct buffer_offset,
                                   struct buffer_dimens,
                                   uint32_t character,
                                   void *);

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
    buffer_release(terminal.buffer);
    
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
    
    double step = 0;
    double interpolate = 0;
    
    timer_begin(terminal.timer); {
        uint16_t ticks = 0;
        
        while (timer_tick(terminal.timer, frequency, &step)) {
            // update input for every tick this frame
            window_read(window, &terminal.keys, &terminal.cursor);
            
            if (terminal.tick_func) {
                terminal.tick_func(step);
            }
            
            ticks += 1;
        }
        
        if (ticks == 0) {
            // make sure to update input at least once per frame
            // (if time has stopped, then input would never be read)
            window_read(window, &terminal.keys, &terminal.cursor);
        }
    }
    timer_end(terminal.timer, &interpolate);
    
    term_handle_internal_input();
    
#ifdef DEBUG
    profiler_begin(); {
#endif
        graphics_begin(terminal.graphics); {
            if (terminal.draw_func) {
                terminal.draw_func(interpolate);
            }
        }
        graphics_end(terminal.graphics);
#ifdef DEBUG
    }
    profiler_end();
#endif
    
    window_present(window);
}

void
term_print(struct term_location const location,
           struct term_color const color,
           struct term_layer const layer,
           char const * const text)
{
    if (!buffer_copy(terminal.buffer, text)) {
        return;
    }
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;
    
    state.x = location.x;
    state.y = location.y;
    state.z = layer_z(layer);

    state.tint = (struct graphics_color) {
        .r = color.r / 255.0f,
        .g = color.g / 255.0f,
        .b = color.b / 255.0f,
        .a = color.a
    };
    
    buffer_characters(terminal.buffer, term_print_character, &state);
}

void
term_measure(char const * const text,
             int32_t * const width,
             int32_t * const height)
{
    if (!buffer_copy(terminal.buffer, text)) {
        return;
    }
    
    // initialize a state for measuring contents of the buffer;
    // this state will hold the smallest possible bounding box that
    // can contain all would-be printed characters
    struct term_state_measure state;
    
    state.width = 0;
    state.height = 0;
    
    buffer_characters(terminal.buffer, term_measure_character, &state);
    
    *width = state.width;
    *height = state.height;
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

void
term_cursor(struct term_cursor_state * const cursor)
{
    cursor->location = terminal.cursor.location;
    cursor->scroll = terminal.cursor.scroll;
    
    // the viewport is needed to transform cursor location from screen-space
    // coordinates to world-space coordinates
    struct viewport const viewport = graphics_get_viewport(terminal.graphics);
    
    // determine backing pixel-scale (to support retina/high-dpi displays)
    float horz_pixel_scale, vert_pixel_scale;
    
    window_get_pixel_scale(terminal.window,
                           &horz_pixel_scale, &vert_pixel_scale);
    
    // offset cursor location by the dimensions taken up by any boxed bars
    cursor->location.x -= (viewport.offset.width / 2) / horz_pixel_scale;
    cursor->location.y -= (viewport.offset.height / 2) / vert_pixel_scale;
    
    // determine pixel sizes (pixels are stretched in fullscreen)
    float horz_pixel_size, vert_pixel_size;

    viewport_pixel_size(viewport, &horz_pixel_size, &vert_pixel_size);

    // scale pixel sizes by the backing pixel-scale
    horz_pixel_size /= horz_pixel_scale;
    vert_pixel_size /= vert_pixel_scale;
    
    // determine aspect ratio to scale cursor location by
    float const aspect = horz_pixel_size > vert_pixel_size ?
        vert_pixel_size : horz_pixel_size;

    // finally determine the cursor location within our world space
    cursor->location.x = (int32_t)(cursor->location.x / aspect);
    cursor->location.y = (int32_t)(cursor->location.y / aspect);
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
 
    terminal.buffer = buffer_init((struct buffer_dimens) {
        IBM8x8_CELL_SIZE,
        IBM8x8_CELL_SIZE
    });
    
    terminal.draw_func = NULL;
    terminal.tick_func = NULL;
    
    load_image_data(IBM8x8_FONT, IBM8x8_LENGTH, term_callback_font_loaded);
    
    term_invalidate();
#ifdef DEBUG
    profiler_reset();
#endif
    return true;
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
term_print_character(struct buffer_offset offset,
                     struct buffer_dimens const dimensions,
                     uint32_t const character,
                     void * const data)
{
    if (character == ' ') {
        // don't draw whitespace
        return;
    }
    
    struct term_state_print * const state = (struct term_state_print *)data;
    
    struct viewport const viewport = graphics_get_viewport(terminal.graphics);
    
    // accumulate it
    offset.y = state->y + offset.y;
    // flip it
    offset.y = viewport.resolution.height - dimensions.height - offset.y;
    
    struct graphics_position position = {
        .x = state->x + offset.x,
        .y = offset.y,
        .z = state->z
    };
    
    graphics_draw(terminal.graphics,
                  state->tint,
                  position,
                  character);
}

static
void
term_measure_character(struct buffer_offset const offset,
                       struct buffer_dimens const dimensions,
                       uint32_t const character,
                       void * const data)
{
    (void)character;
    
    struct term_state_measure * const state = (struct term_state_measure *)data;
    
    int32_t const right = offset.x + dimensions.width;
    int32_t const bottom = offset.y + dimensions.height;
    
    if (right > state->width) {
        state->width = right;
    }
    
    if (bottom > state->height) {
        state->height = bottom;
    }
}

static
void
term_callback_font_loaded(struct graphics_image const image)
{
    struct graphics_font font;
    
    font.codepage = CP437;
    font.columns = IBM8x8_COLUMNS;
    font.rows = IBM8x8_ROWS;
    font.size = IBM8x8_CELL_SIZE;
    
    graphics_set_font(terminal.graphics, image, font);
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
    params->hide_cursor = true;
    
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
