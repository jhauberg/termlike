#pragma once

#include <termlike/config.h> // term_settings :completeness
#include <termlike/input.h> // term_key :completeness
#include <termlike/bounds.h> // term_bounds :completeness
#include <termlike/position.h> // term_position :completeness
#include <termlike/transform.h> // term_transform :completeness
#include <termlike/color.h> // term_color :completeness

#include <stdbool.h> // bool
#include <stdint.h> // uint16_t
#include <stddef.h> // size_t

/**
 * Represents the frequency for a responsive game.
 */
#define TERM_FREQUENCY_DEFAULT 30
/**
 * Represents the frequency for an extremely responsive game.
 */
#define TERM_FREQUENCY_FAST 60
/**
 * Represents the frequency for a game that ticks once every second.
 */
#define TERM_FREQUENCY_ONCE_A_SECOND 1
/**
 * Represents the frequency for a game that does not tick.
 */
#define TERM_FREQUENCY_NONE 0

/**
 * Represents a single glyph.
 *
 * This could be any character, but a whitespace seems appropriate.
 */
#define TERM_SINGLE_GLYPH " "

/**
 * Represents the function signature for a drawing implementation.
 *
 * The interpolation value (a scalar, ranging from 0 to 1) can be used to
 * blend visual game state (movement, animation etc.) from the previous frame
 * to this frame, in order to smooth out otherwise jarring visuals.
 *
 * As an example, some gameplay code may advance the logical state of a
 * player's position very rapidly, but when a bunch of stuff happens that
 * cause the framerate to dip, you would typically see the player jumping
 * from position to position.
 *
 * This can be avoided by blending between the previously rendered position
 * and the actual, logical, position, and provides a smoother looking result.
 */
typedef void term_draw_callback(double interpolation);
/**
 * Represents the function signature for a ticking implementation.
 *
 * The stepping value is a product of the frequency (hz) that the terminal is
 * running at, and represents the time (in seconds) that each tick should
 * apply to the logical game state/timeline.
 */
typedef void term_tick_callback(double step);

/**
 * Open the terminal window with specified settings.
 */
bool term_open(struct term_settings);
/**
 * Close the terminal window.
 */
bool term_close(void);

/**
 * Determine whether the terminal is set to close and should be stopped.
 */
bool term_is_closing(void);
/**
 * Set whether the terminal should be stopped and closed.
 */
void term_set_closing(bool close);

/**
 * Set the drawing function.
 *
 * This function will be called immediately before rendering a frame.
 */
void term_set_drawing(term_draw_callback *);
/**
 * Set the ticking function.
 *
 * This function will be called whenever a logical tick should occur.
 */
void term_set_ticking(term_tick_callback *);

/**
 * Get the resolution of the display (in pixels).
 *
 * The resolution remains the same whether the display is fullscreen or
 * windowed.
 */
void term_get_display(struct term_dimens *);

/**
 * Execute a single frame (read input, process gameplay and render).
 *
 * The frequency (hz) defines the rate at which gameplay ticks should be
 * occurring. It does not affect the rendering framerate.
 *
 * In general, the rendering framerate is always uncapped (unless v-sync is
 * enabled). This is preferable for most displays.
 *
 * Should the framerate ever dip lower than the ticking frequency, several
 * ticks may be issued in rapid succession to keep up with the time that has
 * passed since the last frame was rendered. This makes rendering and gameplay
 * independent of each other, and is required to provide a similar gameplay
 * experience regardless of framerate (slower vs. faster machines).
 *
 * A higher frequency can provide a smoother and more responsive experience,
 * but at the cost of more CPU cycles.
 *
 * Choosing the *right* frequency is always dependent on the kind of game being
 * built. Additionally, gameplay code will be *fully* tied to the chosen
 * frequency, and will feel different should you decide to change it later.
 */
void term_run(uint16_t frequency);

/**
 * Set the glyph transformation.
 */
void term_set_transform(struct term_transform);
/**
 * Get the currently set glyph transformation.
 */
void term_get_transform(struct term_transform *);

/**
 * Print a set of characters.
 */
void term_print(char const * characters,
                struct term_position,
                struct term_color);
/**
 * Print a string.
 *
 * The string is automatically word-wrapped if a bounded area is provided.
 */
void term_printstr(char const * text,
                   struct term_position,
                   struct term_color,
                   struct term_bounds);

/**
 * Count the number of printable characters in a string or set of characters.
 */
void term_count(char const * characters, size_t * amount);
/**
 * Measure the printed dimensions of a set of characters.
 *
 * The resulting dimensions are scaled according to the currently set glyph
 * transformation.
 */
void term_measure(char const * characters,
                  struct term_dimens *);
/**
 * Measure the printed dimensions of a string.
 *
 * The resulting dimensions are scaled according to the currently set glyph
 * transformation.
 */
void term_measurestr(char const * text,
                     struct term_bounds,
                     struct term_dimens *);

/**
 * Determine whether a key is currently held down.
 */
bool term_key_down(enum term_key);
/**
 * Determine whether a key has just been pressed (previously not held down).
 */
bool term_key_pressed(enum term_key);
/**
 * Determine whether a key has just been released (previously held down)
 */
bool term_key_released(enum term_key);

/**
 * Get the current cursor state.
 */
void term_get_cursor(struct term_cursor_state *);

/**
 * Fill a rectangular area with a color.
 *
 * The final shape is affected by the currently set glyph transformation.
 */
void term_fill(struct term_position,
               struct term_dimens,
               struct term_color);
