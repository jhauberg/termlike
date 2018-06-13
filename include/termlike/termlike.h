#pragma once

#include <termlike/config.h> // term_settings :completeness
#include <termlike/input.h> // term_key :completeness

#include <stdbool.h> // bool
#include <stdint.h> // uint16_t

#define TERM_FREQUENCY_DEFAULT 30
#define TERM_FREQUENCY_ONCE_A_SECOND 1

typedef void term_draw_callback(double interpolation);
typedef void term_tick_callback(double step);

bool term_open(struct term_settings);
bool term_close(void);

bool term_is_closing(void);
void term_set_closing(bool close);

void term_set_drawing(term_draw_callback *);
void term_set_ticking(term_tick_callback *);

void term_run(uint16_t frequency);

bool term_key_down(enum term_key);
bool term_key_pressed(enum term_key);
bool term_key_released(enum term_key);

struct term_cursor_state term_cursor(void);
