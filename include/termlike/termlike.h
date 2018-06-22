#pragma once

#include <termlike/config.h> // term_settings :completeness
#include <termlike/input.h> // term_key :completeness
#include <termlike/bounds.h> // term_bounds :completeness
#include <termlike/position.h> // term_position :completeness
#include <termlike/transform.h> // term_transform :completeness
#include <termlike/color.h> // term_color :completeness

#include <stdbool.h> // bool
#include <stdint.h> // uint16_t, int32_t

#define TERM_FREQUENCY_DEFAULT 30
#define TERM_FREQUENCY_FAST 60
#define TERM_FREQUENCY_ONCE_A_SECOND 1

typedef void term_draw_callback(double interpolation);
typedef void term_tick_callback(double step);

bool term_open(struct term_settings);
bool term_close(void);

bool term_is_closing(void);
void term_set_closing(bool close);

void term_set_drawing(term_draw_callback *);
void term_set_ticking(term_tick_callback *);

void term_get_display(int32_t * width, int32_t * height);

void term_run(uint16_t frequency);

void term_print(struct term_position,
                struct term_color,
                char const * characters);
void term_printt(struct term_position,
                 struct term_color,
                 struct term_transform,
                 char const * characters);

void term_printstr(struct term_position,
                   struct term_color,
                   struct term_bounds,
                   char const * text);
void term_printstrt(struct term_position,
                    struct term_color,
                    struct term_bounds,
                    struct term_transform,
                    char const * text);

void term_count(char const * characters, size_t * length);
void term_measure(char const * characters, int32_t * width, int32_t * height);
void term_measurestr(char const * text,
                     struct term_bounds,
                     int32_t * width,
                     int32_t * height);

bool term_key_down(enum term_key);
bool term_key_pressed(enum term_key);
bool term_key_released(enum term_key);

void term_cursor(struct term_cursor_state *);
