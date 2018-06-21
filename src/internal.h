#pragma once

#include <termlike/config.h> // term_size

struct term_layer;

struct window_size;
struct window_params;

float layer_z(struct term_layer);

void term_get_display_size(enum term_size,
                           struct window_size *);
void term_get_display_params(struct term_settings,
                             struct window_size,
                             struct window_params *);
