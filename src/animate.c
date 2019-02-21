#include <termlike/animate.h> // term_animate, animated

#include <stdbool.h> // bool
#include <stdint.h> // int32_t

#include <math.h> // fabsf

extern inline struct term_animate animated(float value);

static void animate_set(struct term_animate *, float value);
static float animate_lerp(float a, float b, float t);

void
animate_to(struct term_animate * const animatable,
           float const value,
           double const duration,
           double const step)
{
    animatable->time += step;

    if (duration > SECONDS(0) && animatable->time < duration) {
        double t = animatable->time;

        if (t < SECONDS(0)) {
            t = SECONDS(0);
        } else if (t > duration) {
            t = duration;
        }

        float const scalar = (float)(t / duration);

        animate_set(animatable, animate_lerp(animatable->value.original,
                                             value,
                                             scalar));
    } else {
        animate_set(animatable, value);
    }
}

void
animate_by(struct term_animate * const animatable,
           float const value,
           double const step)
{
    animatable->value.original += value;

    animate_to(animatable, animatable->value.original, SECONDS(0), step);
}

void
animate_blendf(struct term_animate const animatable,
               double const interpolation,
               float * const value)
{
    *value = animate_lerp(animatable.value.previous,
                          animatable.value.current,
                          interpolation);
}

void
animate_blend(struct term_animate const animatable,
              double const interpolation,
              int32_t * const value)
{
    float v;

    animate_blendf(animatable, interpolation, &v);

    *value = (int32_t)v;
}

static
void
animate_set(struct term_animate * const animatable, float const value)
{
    animatable->value.previous = animatable->value.current;
    animatable->value.current = value;
}

static
float
animate_lerp(float const a, float const b, float const t)
{
    return (1.0f - t) * a + t * b;
}
