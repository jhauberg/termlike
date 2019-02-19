#include <termlike/animate.h> // term_animate, animated

#include <stdlib.h> // malloc, free
#include <stdbool.h> // bool

#include <math.h> // fabsf

struct vector {
    float current;
    float previous;
};

struct term_animate {
    double time;
    float value;
    struct vector result;
};

static float animate_blend_value(float previous, float next, double interp);
static float animate_from_to(float a, float b, double time, double duration);

static bool animate_equals(float a, float b);
static float animate_lerp(float a, float b, float t);

struct term_animate *
animated(float const value)
{
    struct term_animate * const anim = malloc(sizeof(struct term_animate));

    animate_reset(anim, value);

    return anim;
}

void
animate_release(struct term_animate * const animatable)
{
    free(animatable);
}

void
animate_reset(struct term_animate * const animatable, float const value)
{
    animatable->time = 0;
    animatable->value = value;
    animatable->result.current = animatable->value;
    animatable->result.previous = animatable->value;
}

void
animate_set(struct term_animate * const animatable, float const value)
{
    animatable->value = value;
    animatable->result.previous = animatable->result.current;
    animatable->result.current = animatable->value;
}

void
animate_to(struct term_animate * const animatable,
           float const value,
           double const duration,
           double const step)
{
    if (duration > 0) {
        animatable->time += step;

        if (animatable->time > duration) {
            animatable->time = duration;
        }

        animatable->result.previous = animatable->result.current;
        animatable->result.current = animate_from_to(animatable->value,
                                                     value,
                                                     animatable->time,
                                                     duration);
    } else {
        animate_set(animatable, value);
    }
}

void
animate_by(struct term_animate * const animatable,
           float const add,
           double const step)
{
    float const from = animatable->value;
    float const value = from + (float)(add * step);

    animate_set(animatable, value);
}

void
animate_get(struct term_animate const * const animatable, float * const value)
{
    *value = animatable->value;
}

void
animate_blendf(struct term_animate const * const animatable,
               double const interpolation,
               float * const value)
{
    *value = animate_blend_value(animatable->result.previous,
                                 animatable->result.current,
                                 interpolation);
}

void
animate_blend(struct term_animate const * const animatable,
              double const interpolation,
              int32_t * const value)
{
    float v;

    animate_blendf(animatable, interpolation, &v);

    *value = (int32_t)v;
}

static
float
animate_from_to(float const a,
                float const b,
                double const time,
                double const duration)
{
    float const t = duration > 0 ? (float)(time / duration) : 1;

    float d = animate_lerp(a, b, t);

    if (animate_equals(d, b)) {
        d = b;
    }

    return d;
}

static
float
animate_blend_value(float const previous,
                    float const next,
                    double const interpolation)
{
    if (animate_equals(previous, next)) {
        return next;
    }

    float t = (float)interpolation;

    if (t < 0) {
        t = 0;
    } else if (t > 1) {
        t = 1;
    }

    return animate_lerp(previous, next, t);
}

static
bool
animate_equals(float const a, float const b)
{
    static double const precision = 1.0 / 1048;

    return fabsf(b - a) < precision;
}

static
float
animate_lerp(float const a, float const b, float const t)
{
    return (1.0f - t) * a + t * b;
}
