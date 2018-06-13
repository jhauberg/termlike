#pragma once

#include <stdint.h> // uint16_t
#include <stdbool.h> // bool

struct timer;

struct timer * timer_init(void);
void timer_release(struct timer *);

void timer_begin(struct timer *);
void timer_end(struct timer *, double * interpolation);
void timer_reset(struct timer *);

bool timer_tick(struct timer *, uint16_t frequency, double * step);

double timer_time(struct timer const *);
double timer_since(struct timer const *, double time);

void timer_set_scale(struct timer *, double scale);
