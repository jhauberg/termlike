#include <termlike/termlike.h> // term_*
#include <termlike/layer.h> // term_layer
#include <termlike/config.h> // term_settings, term_size
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdint.h> // uint16_t, uint32_t, int32_t
#include <stdbool.h> // bool
#include <string.h> // strncpy, strlen

#include <utf8.h> // utf8_decode

#include "internal.h" // layer_z
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
 * The number of bytes that the internal text buffer must be zero-padded with.
 
 * This is required for UTF8 decoding.
 */
#define BUFFER_PADDING 4
/**
 * The size of the internal text buffer.
 *
 * This value must be at least 1 higher than the amount of required padding.
 */
#define MAX_BUFFER_SIZE 128 // todo: this is low. consider max=width*height
/**
 * The maximum length of a printed string.
 */
#define MAX_TEXT_LENGTH (MAX_BUFFER_SIZE - BUFFER_PADDING)

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
    char buffer[MAX_BUFFER_SIZE];
    struct window_context * window;
    struct graphics_context * graphics;
    struct timer * timer;
    struct term_key_state keys;
    struct term_cursor_state cursor;
};

/**
 * Represents a function that reacts to a character at an offset in a buffer.
 */
typedef void term_character_callback(int32_t, int32_t, uint32_t, void *);

static bool term_setup(struct window_size);
static void term_invalidate(void);

/**
 * Copy a string to the internal buffer, preparing it for drawing.
 *
 * Before copying, the string is validated against buffer limitations.
 * If valid, the string is copied and the buffer is zero-padded appropriately.
 *
 * Return true if string was copied, false otherwise.
 */
static bool term_copy(char const *);

static void term_handle_internal_input(void);

static void term_toggle_fullscreen(void);

static void term_get_display_size(enum term_size, struct window_size *);
static void term_get_display_params(struct term_settings,
                                    struct window_size,
                                    struct window_params *);

/**
 * Run through each printable character in the internal buffer and issue a
 * callback at each offset.
 *
 * The buffer is read from left to right, and newlines do not trigger callbacks.
 *
 * Functions that require stateful unrolls can provide a generic void pointer
 * that will be passed along with each callback.
 */
static void term_characters(term_character_callback *, void *);
/**
 * Print a character at an offset.
 *
 * This function can be passed to `term_characters(..)` as a character callback.
 */
static void term_print_character(int32_t x, int32_t y, uint32_t character, void *);
/**
 * Measure a character at an offset.
 *
 * This function can be passed to `term_characters(..)` as a character callback.
 */
static void term_measure_character(int32_t x, int32_t y, uint32_t character, void *);

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
term_print(int32_t const x,
           int32_t const y,
           struct term_color const color,
           struct term_layer const layer,
           char const * const text)
{
    if (!term_copy(text)) {
        return;
    }
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;
    
    state.x = x;
    state.y = y;
    state.z = layer_z(layer);

    state.tint = (struct graphics_color) {
        .r = color.r / 255.0f,
        .g = color.g / 255.0f,
        .b = color.b / 255.0f,
        .a = color.a
    };
    
    term_characters(term_print_character, &state);
}

void
term_measure(char const * const text,
             int32_t * const width,
             int32_t * const height)
{
    if (!term_copy(text)) {
        return;
    }
    
    // initialize a state for measuring contents of the buffer;
    // this state will hold the smallest possible bounding box that
    // can contain all would-be printed characters
    struct term_state_measure state;
    
    state.width = 0;
    state.height = 0;
    
    term_characters(term_measure_character, &state);
    
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

struct term_cursor_state
term_cursor(void)
{
    struct viewport const viewport = graphics_get_viewport(terminal.graphics);
    
    double const pixel = viewport_pixel(viewport);
    
    struct term_cursor_location location = terminal.cursor.location;
    
    if (pixel > 1) {
        location.x = (int32_t)(location.x / pixel);
        location.y = (int32_t)(location.y / pixel);
    }

    return (struct term_cursor_state) {
        .location = location,
        .scroll = terminal.cursor.scroll
    };
}

static
bool
term_setup(struct window_size const display)
{
    memset(terminal.buffer, '\0', sizeof(terminal.buffer));
    
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
bool
term_copy(char const * const text)
{
    size_t const lenght = strlen(text);
    
    if (lenght >= MAX_TEXT_LENGTH) {
        return false;
    }
    
    strncpy(terminal.buffer, text, lenght);
    
    for (uint16_t i = 0; i < BUFFER_PADDING; i++) {
        // null-terminate the buffer and add padding (utf8 decoding)
        terminal.buffer[lenght + i] = '\0';
    }
    
    return true;
}

static
void
term_characters(term_character_callback * const callback, void * const state)
{
    char * text_ptr = terminal.buffer;
    
    int32_t decoding_error;
    uint32_t character;
    
    int32_t x = 0;
    
    int32_t x_offset = 0;
    int32_t y_offset = 0;
    
    while (*text_ptr) {
        // decode and advance the text pointer
        text_ptr = utf8_decode(text_ptr, &character, &decoding_error);
        
        if (decoding_error != 0) {
            // error
        }
        
        if (character == '\n') {
            y_offset += IBM8x8_CELL_SIZE;
            x_offset = x;
            
            continue;
        } else {
            callback(x_offset, y_offset, character, state);
        }
        
        x_offset += IBM8x8_CELL_SIZE;
    }
}

static
void
term_print_character(int32_t const x,
                     int32_t y,
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
    y = state->y + y;
    // flip it
    y = viewport.resolution.height - IBM8x8_CELL_SIZE - y;
    
    graphics_draw(terminal.graphics,
                  state->tint,
                  state->x + x,
                  y,
                  state->z,
                  character);
}

static
void
term_measure_character(int32_t const x,
                       int32_t const y,
                       uint32_t const character,
                       void * const data)
{
    (void)character;
    
    struct term_state_measure * const state = (struct term_state_measure *)data;
    
    int32_t const right = x + IBM8x8_CELL_SIZE;
    int32_t const bottom = y + IBM8x8_CELL_SIZE;
    
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
