#include <termlike/termlike.h> // term_*
#include <termlike/position.h> // position :completeness
#include <termlike/bounds.h> // term_bounds, TERM_ALIGN_*, TERM_WRAP_*
#include <termlike/transform.h> // term_transform
#include <termlike/config.h> // term_settings
#include <termlike/input.h> // term_key, term_cursor_state
#include <termlike/input/state.h> // term_key_state
#include <termlike/graphics/renderer.h> // graphics_context, graphics_*
#include <termlike/graphics/viewport.h> // viewport
#include <termlike/graphics/loader.h> // load_image_data
#include <termlike/platform/window.h> // window_size, window_params, window_*
#include <termlike/platform/timer.h> // timer, timer_*

#include <termlike/resources/spritefont.8x8.h> // IBM8x8*

#ifdef TERM_INCLUDE_PROFILER
 #include <termlike/platform/profiler.h> // profiler_*
#endif

#include "internal.h" // term_get_display_*
#include "buffer.h" // buffer, buffer_*
#include "command.h" // command_buffer, command, command_*
#include "cursor.h" // cursor, cursor_offset, cursor_*

#include <stdint.h> // uint16_t, uint32_t, int32_t
#include <stddef.h> // size_t, NULL
#include <stdbool.h> // bool

#ifdef _WIN32
 #define _USE_MATH_DEFINES
#endif

#include <math.h> // M_PI, floorf

#ifdef DEBUG
 #include <assert.h> // assert
#endif

#define PIXEL(x) ((int32_t)floorf(x))

#define MAX_LINES (128)

/**
 * Provides a count that can be accumulated for each printable character
 * in a buffer.
 */
struct term_state_count {
    size_t count;
};

/**
 * Provides widths for all lines in a buffer.
 */
struct term_lines {
    // the maximum number of lines is *actually* equal to the maximum size of
    // the internal buffer (in the case where each character/byte is a newline)
    // but that is really a huge amount of lines, so we artificially limit it
    // to much less than that
    int32_t widths[MAX_LINES];
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
 * Provides measured values for the contents of a buffer.
 */
struct term_measurement {
    /**
     * The horizontal dimensions of each line in the measured buffer contents.
     */
    struct term_lines lines;
    /**
     * The dimensions of the smallest bounding box that can hold the measured
     * buffer contents.
     */
    struct term_dimens size;
};

/**
 * Provides values for rendering printable characters in a buffer.
 */
struct term_state_print {
    struct graphics_color tint;
    struct viewport_size display;
    struct cursor cursor;
    struct graphics_position origin;
    struct term_bounds bounds;
    struct term_measurement measured;
    struct term_scale scale;
    struct term_anchor anchor;
    float radians;
    enum term_rotate rotation;
};

struct term_attributes {
    struct term_transform transform;
    // todo: linespacing; maybe just a term_dimens for glyph padding
    //       essentially same as linespacing, but also adds horizonta padding
    // todo: note that this would have to be captured per command, just like transform
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
    struct term_attributes attributes;
    struct term_key_state keys;
    struct term_key_state previous_keys;
    struct term_cursor_state cursor;
    bool is_open;
#ifdef TERM_INCLUDE_PROFILER
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
 * Reads input and calculates deltas since the last frame, then ticks as
 * many times as needed to keep game state in sync.
 */
static void term_update(uint16_t frequency, double * interpolate);
/**
 * Render a frame.
 *
 * Accumulates all print commands, then proceeds to flush them by sorting
 * and drawing each command back-to-front.
 */
static void term_draw(double interpolate);

/**
 * Copy a string to the internal buffer and apply wrapping if needed.
 */
static void term_copy_str(char const * text,
                          struct term_bounds,
                          struct term_scale);

/**
 * Measure the contents of the internal buffer.
 */
static void term_measure_buffer(struct term_bounds,
                                struct term_scale,
                                struct term_measurement *);

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
    
    struct window_size display = (struct window_size) {
        .width = settings.size.width,
        .height = settings.size.height
    };
    
    struct window_params params;
    
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
term_get_display(struct term_dimens * const dimensions)
{
    struct viewport viewport;

    graphics_get_viewport(terminal.graphics, &viewport);

    dimensions->width = viewport.resolution.width;
    dimensions->height = viewport.resolution.height;
}

void
term_run(uint16_t const frequency)
{
    double interpolate = 0;
    
    term_update(frequency, &interpolate);
#ifdef TERM_INCLUDE_PROFILER
    profiler_begin();
#endif
    term_draw(interpolate);
#ifdef TERM_INCLUDE_PROFILER
    profiler_end();
#endif
    
    window_present(terminal.window);
}

void
term_set_transform(struct term_transform const transform)
{
    terminal.attributes.transform = transform;
}

void
term_get_transform(struct term_transform * const transform)
{
    *transform = terminal.attributes.transform;
}

void
term_print(char const * const character,
           struct term_position const position,
           struct term_color const color)
{
#ifdef DEBUG
    assert(character != NULL);
#endif
    struct term_bounds bounds = TERM_BOUNDS_NONE;

    bounds.limit = 1;

    term_printstr(character, position, color, bounds);
}

void
term_printstr(char const * const text,
              struct term_position const position,
              struct term_color const color,
              struct term_bounds const bounds)
{
#ifdef DEBUG
    assert(text != NULL);
#endif
    struct term_transform transform;
    
    term_get_transform(&transform);

    uint32_t const index = command_next_layered_index(terminal.queue,
                                                      position.layer);

    struct command cmd = (struct command) {
        .index = index,
        .text = text,
        .transform = transform,
        .origin = position.location,
        .bounds = bounds,
        .color = color
    };
    
    command_push(terminal.queue, cmd);
}

void
term_fill(struct term_position const position,
          struct term_dimens const size,
          struct term_color const color)
{
    struct graphics_font font;

    graphics_get_font(terminal.graphics, &font);

    float const h = (float)size.width / (float)font.size;
    float const v = (float)size.height / (float)font.size;

    struct term_transform transform;

    term_get_transform(&transform);

    struct term_transform const previous_transform = transform;

    transform.scale.horizontal = h * transform.scale.horizontal;
    transform.scale.vertical = v * transform.scale.vertical;

    term_set_transform(transform);

    term_print("█", position, color);

    term_set_transform(previous_transform);
}

void
term_count(char const * const text, size_t * const length)
{
#ifdef DEBUG
    assert(text != NULL);
#endif
    *length = 0;
    
    // initialize a state for counting number of printable characters
    struct term_state_count state;
    
    state.count = 0;
 
    buffer_copy(terminal.buffer, text, TERM_BOUNDS_UNBOUNDED);
    buffer_foreach(terminal.buffer, term_count_character, &state);
    
    *length = state.count;
}

void
term_measure(char const * const character,
             struct term_dimens * const dimensions)
{
#ifdef DEBUG
    assert(character != NULL);
#endif
    struct term_bounds bounds = TERM_BOUNDS_NONE;

    bounds.limit = 1;

    term_measurestr(character, bounds, dimensions);
}

void
term_measurestr(char const * const text,
                struct term_bounds const bounds,
                struct term_dimens * const dimensions)
{
#ifdef DEBUG
    assert(text != NULL);
#endif
    dimensions->width = 0;
    dimensions->height = 0;
    
    struct term_transform transform;
    
    term_get_transform(&transform);
    
    struct term_scale const scale = transform.scale;
    
    term_copy_str(text, bounds, scale);
    
    struct term_measurement measurement;
    
    term_measure_buffer(bounds, scale, &measurement);
    
    dimensions->width = measurement.size.width;
    dimensions->height = measurement.size.height;
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
term_get_cursor(struct term_cursor_state * const cursor)
{
    cursor->location = terminal.cursor.location;
    cursor->scroll = terminal.cursor.scroll;
    
    struct viewport viewport;

    graphics_get_viewport(terminal.graphics, &viewport);

    // the viewport is needed to transform cursor location from screen-space
    // coordinates to world-space coordinates
    window_translate_cursor(terminal.window, cursor, viewport);
}

static
bool
term_setup(struct window_size const display)
{
    struct viewport viewport;
    
    viewport.offset.width = 0;
    viewport.offset.height = 0;
    
    // initialize framebuffer, actual values set in term_invalidate
    viewport.framebuffer.height = 0;
    viewport.framebuffer.width = 0;
    
    viewport.resolution.width = display.width;
    viewport.resolution.height = display.height;
    
    terminal.graphics = graphics_init(viewport);
    
    if (terminal.graphics == NULL) {
        return false;
    }
    
    term_invalidate();
    
    terminal.timer = timer_init();
    
    if (terminal.timer == NULL) {
        return false;
    }
    
    terminal.queue = command_init();
    terminal.buffer = buffer_init();

    terminal.draw_func = NULL;
    terminal.tick_func = NULL;
    
    load_image_data(IBM8x8_FONT, IBM8x8_SIZE, term_load_font);
    
    term_set_transform(TERM_TRANSFORM_NONE);
#ifdef TERM_INCLUDE_PROFILER
    terminal.is_profiling = false;
    
    profiler_reset();
#endif
    return true;
}

static
void
term_invalidate(void)
{
    struct viewport viewport;
    
    graphics_get_viewport(terminal.graphics, &viewport);
    
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
    
#ifdef TERM_INCLUDE_PROFILER
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
#ifdef TERM_INCLUDE_PROFILER
    float command_load = 0;
#endif
    graphics_begin(terminal.graphics); {
        if (terminal.draw_func) {
            terminal.draw_func(interpolate);
        }
#ifdef TERM_INCLUDE_PROFILER
        if (terminal.is_profiling) {
            profiler_draw();

            size_t used, capacity;

            // fetch size of command buffer as an indication of how much memory
            // is being used to accomodate all print commands for a program
            command_get_capacity(terminal.queue, &used, &capacity);
            command_load = (float)used / capacity;
        }
#endif
        command_flush(terminal.queue, term_print_command);
    }
    graphics_end(terminal.graphics);

#ifdef TERM_INCLUDE_PROFILER
    if (terminal.is_profiling) {
        // sum up stats from this frame
        profiler_sum(profiler_stats(), command_load);
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
term_copy_str(char const * const text,
              struct term_bounds const bounds,
              struct term_scale const scale)
{
    buffer_copy(terminal.buffer, text, bounds.limit);
    
    if (bounds.size.width != TERM_BOUNDS_UNBOUNDED &&
        bounds.wrap == TERM_WRAP_WORDS) {
        struct graphics_font font;

        graphics_get_font(terminal.graphics, &font);

        float const cw = (float)font.size * scale.horizontal;
        
        // determine max number of characters per line
        size_t const limit = (size_t)floorf((float)bounds.size.width / cw);
        
        buffer_wrap(terminal.buffer, limit);
    }
}

static
void
term_measure_buffer(struct term_bounds const bounds,
                    struct term_scale const scale,
                    struct term_measurement * const measurement)
{
    // initialize a state for measuring the smallest bounding box that
    // contains all lines of the text, and is within specified bounds
    struct term_state_measure measure;

    struct graphics_font font;

    graphics_get_font(terminal.graphics, &font);
    
    cursor_start(&measure.cursor,
                 bounds,
                 (float)font.size * scale.horizontal,
                 (float)font.size * scale.vertical);
    
    measure.bounds = bounds;
    
    buffer_foreach(terminal.buffer, term_measure_character, &measure);
    
    uint32_t const line_count = measure.cursor.offset.line + 1;
    
    measurement->lines = measure.lines;
    
    measurement->size.width = 0;
    measurement->size.height = PIXEL((float)line_count * measure.cursor.height);
    
    for (uint32_t i = 0; i < line_count; i++) {
        int32_t const width = measure.lines.widths[i];
        
        if (measurement->size.width < width) {
            measurement->size.width = width;
        }
    }
}

static
void
term_print_character(uint32_t const character, void * const data)
{
    struct term_state_print * const state = (struct term_state_print *)data;
    
    struct cursor_offset offset;
    
    cursor_advance(&state->cursor, &offset, character);
    
    if (character == '\n' ||
        character == ' ') {
        // don't print stuff we don't need to
        return;
    }
    
    if (cursor_is_out_of_bounds(&state->cursor)) {
        // don't draw anything out of bounds
        return;
    }
    
    if (state->bounds.align == TERM_ALIGN_RIGHT) {
        offset.x -= state->measured.lines.widths[offset.line];
    } else if (state->bounds.align == TERM_ALIGN_CENTER) {
        offset.x -= (float)state->measured.lines.widths[offset.line] / 2.0f;
    }
    
    float const w = (float)state->measured.size.width;
    float const h = (float)state->measured.size.height;
    
    float const cw = (float)state->cursor.width;
    float const ch = (float)state->cursor.height;
    
    if ((state->rotation == TERM_ROTATE_STRING ||
         state->rotation == TERM_ROTATE_STRING_ANCHORED) &&
        (state->radians > 0 || state->radians < 0)) {
        struct term_anchor const point = {
            .x = offset.x,
            .y = offset.y
        };
        
        struct term_anchor rotated;
        
        if (state->rotation == TERM_ROTATE_STRING_ANCHORED) {
            float const dx = w * state->anchor.x;
            float const dy = h * state->anchor.y;
            
            struct term_anchor const center = {
                .x = dx - (cw / 2),
                .y = dy - (ch / 2)
            };
            
            rotate_point_center(point, center, -state->radians, &rotated);
        } else {
            rotate_point(point, -state->radians, &rotated);
        }
        
        offset.x = rotated.x;
        offset.y = rotated.y;
    }
    
    offset.x += state->origin.x;
    offset.y += state->origin.y;
    
    if (offset.x + cw < 0 || offset.x > state->display.width ||
        offset.y + ch < 0 || offset.y > state->display.height) {
        // cull unnecessary draws
        return;
    }
    
    struct graphics_transform transform = {
        .position = {
            .x = offset.x,
            .y = offset.y,
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

    struct cursor_offset offset;
    
    cursor_advance(&state->cursor, &offset, character);
    
    float edge = offset.x;
    
    if (character != '\n') {
        edge += state->cursor.width;
    }
    
#ifdef DEBUG
    assert(offset.line < MAX_LINES);
#endif
    
    state->lines.widths[offset.line] = PIXEL(edge);
}

static
void
term_print_command(struct command const * const command)
{
    term_copy_str(command->text,
                  command->bounds,
                  command->transform.scale);
    
    // determine the minimum bounding box and line widths for the current
    // contents of the buffer
    struct term_measurement measurement = { 0 };

    if (command->bounds.align != TERM_ALIGN_LEFT ||
        (command->transform.rotate.angle != 0 &&
         command->transform.rotate.rotation == TERM_ROTATE_STRING_ANCHORED)) {
        term_measure_buffer(command->bounds,
                            command->transform.scale,
                            &measurement);
    }
    
    // initialize a state for printing contents of the buffer;
    // this state will hold positional values for the upper-left origin
    // of the string of characters; each character is drawn at an offset
    // from these initial values
    struct term_state_print state;

    struct graphics_font font;

    graphics_get_font(terminal.graphics, &font);
    
    cursor_start(&state.cursor, command->bounds,
                 (float)font.size * command->transform.scale.horizontal,
                 (float)font.size * command->transform.scale.vertical);
    
    state.measured = measurement;
    state.bounds = command->bounds;
    
    state.origin.x = command->origin.x;
    state.origin.y = command->origin.y;
    state.origin.z = command_index_to_z(command->index);

    struct term_rotation const rotate = command->transform.rotate;
    
    state.rotation = rotate.rotation;
    state.anchor = rotate.anchor;
    
    state.radians = 0;
    
    if (rotate.angle != 0 && rotate.angle != 360) {
        state.radians = (float)((rotate.angle * M_PI) / 180.0f);
    }
    
    state.scale = command->transform.scale;
    
    state.tint = (struct graphics_color) {
        .r = command->color.r,
        .g = command->color.g,
        .b = command->color.b,
        .a = command->color.a
    };

    struct viewport viewport;

    graphics_get_viewport(terminal.graphics, &viewport);

    state.display = viewport.resolution;

    buffer_foreach(terminal.buffer, term_print_character, &state);
}

static
void
term_load_font(struct graphics_image const image)
{
    struct graphics_font font;
    
    font.columns = IBM8x8_COLUMNS;
    font.rows = IBM8x8_ROWS;
    font.size = IBM8x8_CELL_SIZE;
    
    graphics_set_font(terminal.graphics, image, font);
}
