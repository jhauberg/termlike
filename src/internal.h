#pragma once

#include <termlike/config.h> // term_size

struct term_layer;
struct term_location;

struct window_size;
struct window_params;

float layer_z(struct term_layer);

void rotate_point(struct term_location point,
                  struct term_location origin,
                  float angle,
                  struct term_location *);

void term_get_display_size(enum term_size,
                           struct window_size *);
void term_get_display_params(struct term_settings,
                             struct window_size,
                             struct window_params *);
