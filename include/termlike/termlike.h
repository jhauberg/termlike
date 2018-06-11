#pragma once

#include <termlike/config.h> // term_settings
#include <termlike/input.h> // term_key :completeness

#include <stdbool.h> // bool

bool term_open(struct term_settings);
bool term_close(void);

bool term_is_closing(void);
void term_set_closing(bool close);

void term_render(void);

bool term_key_down(enum term_key);
bool term_key_pressed(enum term_key);
bool term_key_released(enum term_key);

struct term_cursor_state term_cursor(void);
