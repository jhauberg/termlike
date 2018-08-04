#pragma once

#include <termlike/config.h> // term_size

#include <linmath/linmath.h> // vec2

struct term_layer;

struct window_size;
struct window_params;

float layer_z(struct term_layer);

void rotate_point(vec2 p, float angle, vec2);
void rotate_point_center(vec2 p, vec2 c, float angle, vec2);

void term_get_display_size(enum term_size,
                           struct window_size *);
void term_get_display_params(struct term_settings,
                             struct window_size,
                             struct window_params *);
