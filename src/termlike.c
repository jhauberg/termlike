#include <termlike/termlike.h> // term_*
#include <termlike/position.h> // term_location, term_position
#include <termlike/bounds.h> // term_bounds
#include <termlike/transform.h> // term_transform
#include <termlike/config.h> // term_settings
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdint.h> // uint16_t, uint32_t, int32_t, size_t
#include <stdbool.h> // bool
#include <stdio.h> // sprintf

#include <math.h> // M_PI

#include "internal.h" // layer_z
#include "buffer.h" // buffer, buffer_*
#include "cursor.h" // cursor, cursor_*
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
 * Provides a count that can be accumulated for each printable character
 * in a buffer.
 */
struct term_state_count {
    size_t count;
};

/**
 * Provides values for accumulating the required dimensions of
 * printable characters in a buffer.
 */
struct term_state_measure {
    struct cursor cursor;
    struct term_bounds bounds;
    int32_t width;
    int32_t height;
};

/**
 * Provides initial values for positioning and coloring of printable
 * characters in a buffer.
 */
struct term_state_print {
    struct graphics_color tint;
    struct cursor cursor;
    struct term_location origin;
    struct term_bounds bounds;
    float z;
    float scale;
    float radians;
    enum term_rotate rotation;
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
#ifdef DEBUG
    char profiling_buffer[64];
    bool is_profiling;
#endif
};

static bool term_setup(struct window_size);
static void term_invalidate(void);

static void term_handle_internal_input(void);

#ifdef DEBUG
static void term_toggle_profiling(void);
#endif
static void term_toggle_fullscreen(void);

/**
 * Print a character at an offset.
 *
 * This function can be passed to a buffer as a character callback.
 */
static void term_print_character(uint32_t character, void *);
/**
 * Count a character in a buffer.
 *
 * This function can be passed to a buffer as a character callback.
 */
static void term_count_character(uint32_t character, void *);
/**
 * Measure a character at an offset.
 *
 * This function can be passed to a buffer as a character callback.
 */
static void term_measure_character(uint32_t character, void *);

static void term_load_font(struct graphics_image);

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
term_get_display(int32_t * const width, int32_t * const height)
{
    struct viewport const viewport = graphics_get_viewport(terminal.graphics);
    
    *width = viewport.resolution.width;
    *height = viewport.resolution.height;
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
    if (terminal.is_profiling) {
        // note that we're fetching stats from *previous* frame
        // this is necessary to be certain that everything has flushed
        // and counts are correct (includes profiling visuals)
        struct profiler_stats stats = profiler_stats();
        
        sprintf(terminal.profiling_buffer,
                "%d (%d)",
                stats.frames_per_second_avg,
                stats.draw_count);
        
        profiler_begin();
    }
#endif
        graphics_begin(terminal.graphics); {
            if (terminal.draw_func) {
                terminal.draw_func(interpolate);
            }
            
#ifdef DEBUG
            if (terminal.is_profiling) {
                int32_t w, h;
                
                term_get_display(&w, &h);
                
                char const * const background = "█";
                
                int32_t cw, ch;
                
                term_measure(background, &cw, &ch);
                
                int32_t const columns = w / cw;
                
                for (int32_t i = 0; i < columns; i++) {
                    term_print(positionedz(i * cw, h-ch, layered_below(TERM_LAYER_TOP)),
                               colored(255, 255, 225),
                               background);
                }
                
                term_printstr(positionedz(0, h-ch+1, TERM_LAYER_TOP),
                              colored(55, 55, 55), TERM_BOUNDS_NONE,
                              "` to disable");
                
                term_measurestr(terminal.profiling_buffer, TERM_BOUNDS_NONE, &cw, &ch);
                
                term_printstr(positionedz(w - cw, h-ch+1, TERM_LAYER_TOP),
                              colored(55, 55, 55), TERM_BOUNDS_NONE,
                              terminal.profiling_buffer);
            }
#endif
        }
        graphics_end(terminal.graphics);
#ifdef DEBUG
    if (terminal.is_profiling) {
        profiler_end();
    }
#endif
    
    window_present(window);
}

void
term_print(struct term_position const position,
           struct term_color const color,
           char const * const characters)
{
    term_printstr(position, color, TERM_BOUNDS_NONE, characters);
}

void
term_printt(struct term_position const position,
            struct term_color const color,
            struct term_transform const transform,
            char const * const characters)
{
    term_printstrt(position, color, TERM_BOUNDS_NONE, transform, characters);
}

void
term_printstr(struct term_position const position,
              struct term_color const color,
              struct term_bounds const bounds,
              char const * const text)
{
    term_printstrt(position, color, bounds, TERM_TRANSFORM_NONE, text);
}

static
void
term_wrap_buffer_if_needed(struct term_bounds const bounds)
{
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    if (bounds.width != TERM_BOUNDS_UNBOUNDED) {
        if (bounds.wrap == TERM_WRAP_WORDS) {
            size_t const limit = (size_t)(bounds.width / font.size);

            buffer_wrap(terminal.buffer, limit);
        }
    }
}

void
term_printstrt(struct term_position const position,
               struct term_color const color,
               struct term_bounds const bounds,
               struct term_transform const transform,
               char const * const text)
{
    buffer_copy(terminal.buffer, text);
    
    term_wrap_buffer_if_needed(bounds);
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;
    
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    cursor_start(&state.cursor, font.size, font.size);
    
    state.origin = position.location;
    state.bounds = bounds;
    
    state.z = layer_z(position.layer);
    
    state.rotation = transform.rotation;
    state.radians = (float)((transform.angle * M_PI) / 180.0);
    state.scale = transform.scale;
    
    state.tint = (struct graphics_color) {
        .r = color.r / 255.0f,
        .g = color.g / 255.0f,
        .b = color.b / 255.0f,
        .a = color.a
    };
    
    buffer_foreach(terminal.buffer, term_print_character, &state);
}

void
term_count(char const * const text,
           size_t * const length)
{
    buffer_copy(terminal.buffer, text);
    
    // initialize a state for counting number of printable characters
    struct term_state_count state;
    
    state.count = 0;
    
    buffer_foreach(terminal.buffer, term_count_character, &state);
    
    *length = state.count;
}

void
term_measure(char const * const characters,
             int32_t * const width,
             int32_t * const height)
{
    term_measurestr(characters, TERM_BOUNDS_NONE, width, height);
}

void
term_measurestr(char const * const text,
                struct term_bounds const bounds,
                int32_t * const width,
                int32_t * const height)
{
    buffer_copy(terminal.buffer, text);
    
    term_wrap_buffer_if_needed(bounds);
    
    // initialize a state for measuring contents of the buffer;
    // this state will hold the smallest possible bounding box that
    // can contain all would-be printed characters
    struct term_state_measure state;
    
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    cursor_start(&state.cursor, font.size, font.size);
    
    state.width = 0;
    state.height = 0;
    
    state.bounds = bounds;
    
    buffer_foreach(terminal.buffer, term_measure_character, &state);
    
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
 
    terminal.buffer = buffer_init();

    terminal.draw_func = NULL;
    terminal.tick_func = NULL;
    
    load_image_data(IBM8x8_FONT, IBM8x8_LENGTH, term_load_font);
    
    term_invalidate();
#ifdef DEBUG
    terminal.is_profiling = false;
    
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

#ifdef DEBUG
    if (term_key_pressed((enum term_key)TERM_KEY_TOGGLE_PROFILING)) {
        term_toggle_profiling();
    }
#endif
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
#ifdef DEBUG
static
void
term_toggle_profiling(void)
{
    terminal.is_profiling = !terminal.is_profiling;
}
#endif
static
void
term_print_character(uint32_t const character, void * const data)
{
    struct term_state_print * const state = (struct term_state_print *)data;
    
    struct term_location location;
    
    // scale up the offset vector so that characters are spaced as expected
    location.x = (int32_t)(state->cursor.offset.x * state->scale);
    location.y = (int32_t)(state->cursor.offset.y * state->scale);
    
    // advance cursor for the next character
    cursor_advance(&state->cursor, state->bounds, character);

    if (character == '\n' ||
        character == ' ') {
        // don't print stuff we don't need to
        return;
    }

    if (cursor_is_out_of_bounds(&state->cursor, state->bounds)) {
        // don't draw anything out of bounds
        return;
    }
    
    if (state->rotation == TERM_ROTATE_STRING) {
        rotate_point(location, state->origin, -state->radians,
                     &location);
    } else {
        location.x += state->origin.x;
        location.y += state->origin.y;
    }
    
    struct graphics_position position = {
        .x = location.x,
        .y = location.y,
        .z = state->z
    };
    
    graphics_draw(terminal.graphics,
                  state->tint,
                  position,
                  // note that each char is also individually rotated
                  state->radians,
                  // and scaled
                  state->scale,
                  character);
}

static
void
term_count_character(uint32_t const character, void * const data)
{
    (void)character;
    
    struct term_state_count * const state = (struct term_state_count *)data;
    
    state->count += 1;
}

static
void
term_measure_character(uint32_t const character, void * const data)
{
    struct term_state_measure * const state = (struct term_state_measure *)data;

    cursor_advance(&state->cursor, state->bounds, character);
    
    if (cursor_is_out_of_bounds(&state->cursor, state->bounds)) {
        // don't measure further than the specified bounds
        return;
    }
    
    int32_t const right = state->cursor.offset.x;
    int32_t const bottom = state->cursor.offset.y + state->cursor.height;
    
    if (right > state->width) {
        state->width = right;
    }
    
    if (bottom > state->height) {
        state->height = bottom;
    }
}

static
void
term_load_font(struct graphics_image const image)
{
    struct graphics_font font;
    
    font.codepage = CP437;
    font.columns = IBM8x8_COLUMNS;
    font.rows = IBM8x8_ROWS;
    font.size = IBM8x8_CELL_SIZE;
    
    graphics_set_font(terminal.graphics, image, font);
}
