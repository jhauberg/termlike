#include <termlike/termlike.h> // term_*
#include <termlike/position.h> // term_location, term_position
#include <termlike/bounds.h> // term_bounds, TERM_ALIGN_*, TERM_WRAP_*
#include <termlike/transform.h> // term_transform
#include <termlike/config.h> // term_settings
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdlib.h> // NULL
#include <stdint.h> // uint16_t, uint32_t, int32_t, size_t
#include <stdbool.h> // bool

#include <math.h> // M_PI, floorf

#include "internal.h" // term_get_display_*
#include "buffer.h" // buffer, buffer_*, BUFFER_SIZE_MAX
#include "command.h" // command_buffer, command, command_*
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
 * Provides measured widths for all lines that can fit in the internal buffer.
 */
struct term_lines {
    // the maximum number of lines will be equal to the maximum size of the
    // internal buffer (because each character/byte could be a newline)
    int32_t widths[BUFFER_SIZE_MAX];
};

/**
 * Provides values for accumulating the required dimensions of
 * printable characters in a buffer.
 */
struct term_state_measure {
    struct cursor cursor;
    struct term_bounds bounds;
    struct term_lines lines;
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
    struct term_lines lines;
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
    struct command_buffer * queue;
    struct term_key_state keys;
    struct term_cursor_state cursor;
#ifdef DEBUG
    bool is_profiling;
#endif
};

static bool term_setup(struct window_size);
static void term_invalidate(void);

static void term_handle_internal_input(void);

/**
 * Copy a string to the internal buffer and apply wrapping.
 */
static void term_buffer_str(char const * text, struct term_bounds);

static void term_toggle_fullscreen(void);

/**
 * Handle a command.
 *
 * This function can be passed to a command buffer when flushing.
 */
static void term_print_command(struct command const *);

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
 * Measure a character in a buffer.
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
    command_release(terminal.queue);
    buffer_release(terminal.buffer);
    
    window_terminate(terminal.window);
 
    terminal.graphics = NULL;
    terminal.buffer = NULL;
    terminal.queue = NULL;
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
    profiler_begin();
#endif
    graphics_begin(terminal.graphics); {
        if (terminal.draw_func) {
            terminal.draw_func(interpolate);
        }
        
#ifdef DEBUG
        if (terminal.is_profiling) {
            // note that we're drawing stats from the previous frame
            profiler_draw();
        }
#endif
        // flush all print commands
        command_flush(terminal.queue, term_print_command);
    }
    graphics_end(terminal.graphics);
#ifdef DEBUG
    profiler_end();
    
    size_t bytes;
    
    // fetch size of command buffer as an indication of how much memory
    // is being used to accomodate all print commands for a program
    command_memuse(terminal.queue, &bytes);
    
    // sum up stats from this frame
    profiler_sum(profiler_stats(), bytes);
#endif
    
    window_present(window);
}

void
term_print(struct term_position const position,
           struct term_color const color,
           char const * const characters)
{
    term_printt(position, color,
                TERM_TRANSFORM_NONE,
                characters);
}

void
term_printt(struct term_position const position,
            struct term_color const color,
            struct term_transform const transform,
            char const * const characters)
{
    term_printstrt(position, color,
                   TERM_BOUNDS_NONE,
                   transform,
                   characters);
}

void
term_printstr(struct term_position const position,
              struct term_color const color,
              struct term_bounds const bounds,
              char const * const text)
{
    term_printstrt(position, color,
                   bounds,
                   TERM_TRANSFORM_NONE,
                   text);
}

void
term_printstrt(struct term_position const position,
               struct term_color const color,
               struct term_bounds const bounds,
               struct term_transform const transform,
               char const * const text)
{
    command_push(terminal.queue, (struct command) {
        .position = position,
        .color = color,
        .bounds = bounds,
        .transform = transform,
        .text = text
    });
}

void
term_count(char const * const text,
           size_t * const length)
{
    // initialize a state for counting number of printable characters
    struct term_state_count state;
    
    state.count = 0;
 
    buffer_copy(terminal.buffer, text);
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
    term_buffer_str(text, bounds);
    
    // initialize a state for measuring the smallest bounding box that
    // contains all lines of the text, and is within specified bounds
    struct term_state_measure measure;
    
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    cursor_start(&measure.cursor, font.size, font.size);
    
    measure.bounds = bounds;
    
    buffer_foreach(terminal.buffer, term_measure_character, &measure);
    
    int32_t const line_count = measure.cursor.breaks + 1;
    
    *width = 0;
    *height = line_count * measure.cursor.height;
    
    for (int32_t i = 0; i < line_count; i++) {
        int32_t const line_width = measure.lines.widths[i];
        
        if (*width < line_width) {
            *width = line_width;
        }
    }
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
    
    terminal.queue = command_init();
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
        terminal.is_profiling = !terminal.is_profiling;
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

static
void
term_buffer_str(char const * const text, struct term_bounds const bounds)
{
    buffer_copy(terminal.buffer, text);
    
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    if (bounds.width != TERM_BOUNDS_UNBOUNDED) {
        if (bounds.wrap == TERM_WRAP_WORDS) {
            size_t const limit = (size_t)(bounds.width / font.size);
            
            buffer_wrap(terminal.buffer, limit);
        }
    }
}

static
void
term_print_character(uint32_t const character, void * const data)
{
    struct term_state_print * const state = (struct term_state_print *)data;
    
    struct term_location location;
    
    // scale up the offset vector so that characters are spaced as expected
    // (note that we grab location before advancing the cursor)
    location.x = (int32_t)(state->cursor.offset.x * state->scale);
    location.y = (int32_t)(state->cursor.offset.y * state->scale);
    
    int32_t const line_index = state->cursor.breaks;
    
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
    
    if (state->bounds.align == TERM_ALIGN_RIGHT) {
        location.x -= state->lines.widths[line_index];
    } else if (state->bounds.align == TERM_ALIGN_CENTER) {
        location.x -= floorf(state->lines.widths[line_index] / 2);
    }
    
    if (state->rotation == TERM_ROTATE_STRING) {
        rotate_point(location, state->origin, -state->radians,
                     &location);
    } else {
        location.x += state->origin.x;
        location.y += state->origin.y;
    }
        
    struct graphics_transform transform = {
        .position = {
            .x = location.x,
            .y = location.y,
            .z = state->z
        },
        .angle = state->radians,
        .horizontal_scale = state->scale,
        .vertical_scale = state->scale
    };
    
    graphics_draw(terminal.graphics,
                  state->tint,
                  transform,
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

    if (cursor_is_out_of_bounds(&state->cursor, state->bounds)) {
        // don't measure further than the specified bounds
        return;
    }
    
    int32_t const line_index = state->cursor.breaks;
    
    // apply line width immediately, before advancing the cursor
    // (some lines might break immediately and advancing would increment
    // the line index, causing the previous line width to be undefined)
    state->lines.widths[line_index] = state->cursor.offset.x;
    
    cursor_advance(&state->cursor, state->bounds, character);
    
    if (character != '\n' && state->cursor.breaks != line_index) {
        // a break was required, but not explicit, so pad previous line width
        // (note this should only be a thing for character wrapped text)
        state->lines.widths[line_index] += state->cursor.width;
    }
  
    // apply width again for the current line
    // (it might be the same line, but it could also be the next one)
    state->lines.widths[state->cursor.breaks] = state->cursor.offset.x;
}

static
void
term_print_command(struct command const * const command)
{
    term_buffer_str(command->text, command->bounds);
    
    // initialize a state for measuring line widths
    // this is needed to be able to align lines horizontally
    struct term_state_measure measure;
    
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    cursor_start(&measure.cursor, font.size, font.size);
    
    measure.bounds = command->bounds;
    
    buffer_foreach(terminal.buffer, term_measure_character, &measure);
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;
    
    cursor_start(&state.cursor, font.size, font.size);
    
    state.origin = command->position.location;
    state.lines = measure.lines;
    state.bounds = command->bounds;
    
    struct command_index const * const index = command_index(command);
    
    state.z = index->z;
    
    state.rotation = command->transform.rotation;
    state.radians = (float)((command->transform.angle * M_PI) / 180.0);
    state.scale = command->transform.scale;
    
    state.tint = (struct graphics_color) {
        .r = command->color.r / 255.0f,
        .g = command->color.g / 255.0f,
        .b = command->color.b / 255.0f,
        .a = command->color.a
    };
    
    buffer_foreach(terminal.buffer, term_print_character, &state);
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
