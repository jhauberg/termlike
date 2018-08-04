#include <termlike/termlike.h> // term_*
#include <termlike/position.h> // term_position
#include <termlike/bounds.h> // term_bounds, TERM_ALIGN_*, TERM_WRAP_*
#include <termlike/transform.h> // term_transform
#include <termlike/config.h> // term_settings
#include <termlike/input.h> // term_key, term_cursor_state

#include <stdint.h> // uint16_t, uint32_t, int32_t
#include <stddef.h> // size_t, NULL
#include <stdbool.h> // bool

#ifdef _WIN32
 #define _USE_MATH_DEFINES
#endif

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
    // the maximum number of lines is equal to the maximum size of the
    // internal buffer (in the case where each character/byte is a newline)
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
    struct viewport_size display;
    struct cursor cursor;
    struct graphics_position origin;
    struct term_bounds bounds;
    struct term_lines lines;
    struct term_dimens size;
    struct term_scale scale;
    struct term_anchor anchor;
    float radians;
    enum term_rotate rotation;
};

/**
 * Represents a Termlike display.
 */
struct term_context {
    term_draw_callback * draw_func;
    term_tick_callback * tick_func;
    struct window_context * window;
    struct graphics_context * graphics;
    struct timer * timer;
    struct buffer * buffer;
    struct command_buffer * queue;
    struct term_key_state keys;
    struct term_key_state previous_keys;
    struct term_cursor_state cursor;
    bool is_open;
#ifdef DEBUG
    bool is_profiling;
#endif
};

/**
 * Initialize the terminal display at a given client size (in pixels).
 *
 * This includes initializing a renderer, timer and buffers.
 */
static bool term_setup(struct window_size);
/**
 * Invalidate the terminal display.
 *
 * This should be called whenever the window size of the display changes.
 *
 * Currently only needed when switching between fullscreen and windowed modes.
 */
static void term_invalidate(void);

/**
 * Update a frame.
 *
 * This reads input and calculate deltas since the last frame, then ticks as
 * many times as needed to keep game state in sync.
 */
static void term_update(uint16_t frequency, double * interpolate);
/**
 * Render a frame.
 *
 * This accumulates all print commands then proceeds to flush them by sorting
 * and drawing each command back-to-front.
 */
static void term_draw(double interpolate);

/**
 * Copy a string to the internal buffer and apply wrapping if needed.
 */
static void term_copy_str(char const * text,
                          struct term_bounds);

/**
 * Toggle between fullscreen and windowed mode for the display.
 *
 * Fullscreen mode applies horizontal or vertical bars if needed to retain
 * the aspect ratio of the display resolution.
 */
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

/**
 * Handle a font image being loaded into memory.
 *
 * This function can be passed to `load_image_data` as a callback.
 *
 * Note that the image data is released immediately after this function has
 * completed.
 */
static void term_load_font(struct graphics_image);

/**
 * The one and only terminal object.
 */
static struct term_context terminal;

bool
term_open(struct term_settings const settings)
{
    if (terminal.is_open) {
        term_close();
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
    
    terminal.is_open = true;
    
    return terminal.is_open;
}

bool
term_close(void)
{
    if (!terminal.is_open) {
        return false;
    }
    
    graphics_release(terminal.graphics);
    timer_release(terminal.timer);
    command_release(terminal.queue);
    buffer_release(terminal.buffer);
    
    window_terminate(terminal.window);
    
    terminal = (struct term_context const) { 0 };

    return !terminal.is_open;
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
    double interpolate = 0;
    
    term_update(frequency, &interpolate);
#ifdef DEBUG
    profiler_begin();
#endif
    term_draw(interpolate);
#ifdef DEBUG
    profiler_end();
    
    size_t bytes;
    
    // fetch size of command buffer as an indication of how much memory
    // is being used to accomodate all print commands for a program
    command_memuse(terminal.queue, &bytes);
    
    // sum up stats from this frame
    profiler_sum(profiler_stats(), bytes);
#endif
    
    window_present(terminal.window);
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
        .x = position.location.x,
        .y = position.location.y,
        .color = color,
        .bounds = bounds,
        .transform = transform,
        .layer = position.layer,
        .text = text
    });
}

void
term_count(char const * const text, size_t * const length)
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
term_measurec(int32_t * const width,
              int32_t * const height)
{
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    *width = font.size;
    *height = font.size;
}

void
term_measurestr(char const * const text,
                struct term_bounds const bounds,
                int32_t * const width,
                int32_t * const height)
{
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    term_copy_str(text, bounds);
    
    // initialize a state for measuring the smallest bounding box that
    // contains all lines of the text, and is within specified bounds
    struct term_state_measure measure;
    
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
    
    window_translate_cursor(terminal.window, cursor, viewport);
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
term_update(uint16_t const frequency, double * const interpolate)
{
    struct window_context * const window = terminal.window;
    
    // read input state for this frame
    // from this point on, input deltas branch off into two synchronized paths;
    // one for frame related input (e.g. draw, internals)
    // and one for tick related input (e.g. tick)
    window_read(window, &terminal.keys, &terminal.cursor);
    
    // save a copy of the current key state for this frame
    struct term_key_state const current_keys = terminal.keys;
    
    if (term_key_released((enum term_key)TERM_KEY_TOGGLE_FULLSCREEN)) {
        term_toggle_fullscreen();
    }
    
#ifdef DEBUG
    if (term_key_pressed((enum term_key)TERM_KEY_TOGGLE_PROFILING)) {
        terminal.is_profiling = !terminal.is_profiling;
    }
#endif
    
    // reset input state as it were during the previous tick
    // note that this is necessary to syncronize input between ticks and not
    // between frames (a game may run so fast that it skips ticks during a
    // frame, and in this case the input delta would be invalid; e.g. you might
    // want to do something on pressed/released, but those states could never
    // trigger if it happened during a frame that skipped a tick)
    terminal.keys = terminal.previous_keys;
    
    timer_begin(terminal.timer); {
        double step = 0;
        
        while (timer_tick(terminal.timer, frequency, &step)) {
            // update input state from previous tick
            window_read(window, &terminal.keys, &terminal.cursor);
            // save a copy of the state read during this tick, so that
            // following ticks use the correct input delta
            terminal.previous_keys = terminal.keys;
            
            if (terminal.tick_func) {
                terminal.tick_func(step);
            }
        }
    }
    timer_end(terminal.timer, interpolate);
    
    // finally set key state to that of this frame
    // (regardless of whether or not a tick occurred during this frame)
    terminal.keys = current_keys;
}

static
void
term_draw(double const interpolate)
{
    graphics_begin(terminal.graphics); {
        if (terminal.draw_func) {
            terminal.draw_func(interpolate);
        }
#ifdef DEBUG
        if (terminal.is_profiling) {
            profiler_draw();
        }
#endif
        command_flush(terminal.queue, term_print_command);
    }
    graphics_end(terminal.graphics);
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
term_copy_str(char const * const text,
              struct term_bounds const bounds)
{
    buffer_copy(terminal.buffer, text);
    
    if (bounds.size.width != TERM_BOUNDS_UNBOUNDED) {
        if (bounds.wrap == TERM_WRAP_WORDS) {
            struct graphics_font const font =
                graphics_get_font(terminal.graphics);
            
            // determine max number of characters per line
            size_t const limit = (size_t)(bounds.size.width / font.size);
            
            buffer_wrap(terminal.buffer, limit);
        }
    }
}

static
void
term_print_character(uint32_t const character, void * const data)
{
    struct term_state_print * const state = (struct term_state_print *)data;
    
    // grab line index before advancing cursor (it might break on next advance)
    int32_t const line_index = state->cursor.breaks;
    
    // scale up the offset vector so that characters are spaced as expected
    // (note that we grab location before advancing the cursor)
    float x = (float)state->cursor.offset.x * state->scale.horizontal;
    float y = (float)state->cursor.offset.y * state->scale.vertical;
    
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
        x -= state->lines.widths[line_index];
    } else if (state->bounds.align == TERM_ALIGN_CENTER) {
        x -= floorf((float)state->lines.widths[line_index] / 2.0f);
    }
    
    float const w = (float)state->size.width * state->scale.horizontal;
    float const h = (float)state->size.height * state->scale.vertical;
    
    float const cw = (float)state->cursor.width * state->scale.horizontal;
    float const ch = (float)state->cursor.height * state->scale.vertical;
    
    if ((state->rotation == TERM_ROTATE_STRING ||
         state->rotation == TERM_ROTATE_STRING_ANCHORED) &&
        (state->radians > 0 || state->radians < 0)) {
        vec2 p = { x, y };
        vec2 rotated;
        
        if (state->rotation == TERM_ROTATE_STRING_ANCHORED) {
            float const dx = w * state->anchor.x;
            float const dy = h * state->anchor.y;
            
            vec2 c = {
                dx - (cw / 2),
                dy - (ch / 2)
            };
            
            rotate_point_center(p, c, -state->radians, rotated);
        } else {
            rotate_point(p, -state->radians, rotated);
        }
        
        x = rotated[0];
        y = rotated[1];
    }
    
    x += state->origin.x;
    y += state->origin.y;
    
    if (x + cw < 0 || x > state->display.width ||
        y + ch < 0 || y > state->display.height) {
        // cull unnecessary draws
        return;
    }
    
    struct graphics_transform transform = {
        .position = {
            .x = x,
            .y = y,
            .z = state->origin.z
        },
        .scale = {
            .horizontal = state->scale.horizontal,
            .vertical = state->scale.vertical
        },
        .angle = state->radians
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
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    term_copy_str(command->text, command->bounds);
    
    // initialize a state for measuring line widths
    // this is needed to be able to align lines horizontally
    // note that we don't use term_measurestr because we don't want it
    // doing any term_copy_str
    
    struct term_state_measure measure;
    
    cursor_start(&measure.cursor, font.size, font.size);
    
    measure.bounds = command->bounds;
    
    buffer_foreach(terminal.buffer, term_measure_character, &measure);
    
    int32_t w, h;
    
    term_measurestr(command->text, command->bounds, &w, &h);
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;
    
    cursor_start(&state.cursor, font.size, font.size);
    
    state.size.width = w;
    state.size.height = h;
    state.lines = measure.lines;
    state.bounds = command->bounds;
    
    struct command_index const * const index = command_index(command);
    
    state.origin.x = command->x;
    state.origin.y = command->y;
    state.origin.z = index->z;
    
    struct term_rotation const rotate = command->transform.rotate;
    
    state.rotation = rotate.rotation;
    state.anchor = rotate.anchor;
    
    if (rotate.angle != 0 && rotate.angle != 360) {
        state.radians = (float)((rotate.angle * M_PI) / 180.0);
    } else {
        state.radians = 0;
    }
    
    state.scale = command->transform.scale;
    
    state.tint = (struct graphics_color) {
        .r = command->color.r / 255.0f,
        .g = command->color.g / 255.0f,
        .b = command->color.b / 255.0f,
        .a = command->color.a
    };
    
    struct viewport const viewport = graphics_get_viewport(terminal.graphics);
    
    state.display = viewport.resolution;
    
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

void
term_fill(struct term_position const position,
          struct term_dimens const size,
          struct term_color const color)
{
    term_fillt(position, size, color, TERM_TRANSFORM_NONE);
}

void
term_fillt(struct term_position const position,
           struct term_dimens const size,
           struct term_color const color,
           struct term_transform transform)
{
    struct graphics_font const font = graphics_get_font(terminal.graphics);
    
    float const h = (float)size.width / (float)font.size;
    float const v = (float)size.height / (float)font.size;
    
    transform.scale.horizontal = h * transform.scale.horizontal;
    transform.scale.vertical = v * transform.scale.vertical;
    
    term_printt(position, color, transform, "█");
}
