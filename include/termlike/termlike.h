#pragma once

#include <termlike/config.h> // term_settings :completeness
#include <termlike/input.h> // term_key :completeness
#include <termlike/layer.h> // term_layer :completeness
#include <termlike/color.h> // term_color :completeness

#include <stdbool.h> // bool
#include <stdint.h> // uint16_t, int32_t

#define TERM_FREQUENCY_DEFAULT 30
#define TERM_FREQUENCY_ONCE_A_SECOND 1

struct term_location {
    int32_t x, y;
};

static inline
struct term_location
located(int32_t const x, int32_t const y)
{
    return (struct term_location) {
        .x = x,
        .y = y
    };
}

typedef void term_draw_callback(double interpolation);
typedef void term_tick_callback(double step);

bool term_open(struct term_settings);
bool term_close(void);

bool term_is_closing(void);
void term_set_closing(bool close);

void term_set_drawing(term_draw_callback *);
void term_set_ticking(term_tick_callback *);

void term_run(uint16_t frequency);

void term_print(struct term_location,
                struct term_color,
                struct term_layer,
                char const * text);

void term_measure(char const * text,
                  int32_t * width,
                  int32_t * height);

bool term_key_down(enum term_key);
bool term_key_pressed(enum term_key);
bool term_key_released(enum term_key);

void term_cursor(struct term_cursor_state *);
