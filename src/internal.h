#pragma once

#include <termlike/config.h> // term_settings :completeness

struct term_layer;
struct term_anchor;

struct window_size;
struct window_params;

void rotate_point(struct term_anchor point, float angle,
                  struct term_anchor *);
void rotate_point_center(struct term_anchor point,
                         struct term_anchor center, float angle,
                         struct term_anchor *);

void term_get_display_params(struct term_settings,
                             struct window_size,
                             struct window_params *);
