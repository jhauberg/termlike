#pragma once

#include <stdint.h> // int32_t

struct term_animate;

struct term_animate * animated(float value);
void animate_release(struct term_animate *);

void animate_get(struct term_animate const *, float * value);
void animate_set(struct term_animate *, float value);

void animate_by(struct term_animate *, float value, double s);
void animate_to(struct term_animate *, float value, double duration, double s);

void animate_blend(struct term_animate const *, double interp, int32_t * value);
void animate_blendf(struct term_animate const *, double interp, float * value);

void animate_reset(struct term_animate *, float value);
