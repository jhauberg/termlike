#include <termlike/animate.h> // term_animate, animated

#include <stdbool.h> // bool

#include <math.h> // fabsf

extern inline struct term_animate animated(float value);

static void animate_set(struct term_animate *, float value);

static float animate_blend_value(float previous, float next, double interp);
static float animate_from_to(float a, float b, double time, double duration);

static bool animate_equals(float a, float b);
static float animate_lerp(float a, float b, float t);

void
animate_to(struct term_animate * const animatable,
           float const value,
           double const duration,
           double const step)
{
    animatable->time += step;

    if (duration > SECONDS(0)) {
        double t = animatable->time;

        if (t < SECONDS(0)) {
            t = SECONDS(0);
        } else if (t > duration) {
            t = duration;
        }

        animate_set(animatable, animate_from_to(animatable->value.original,
                                                value,
                                                t,
                                                duration));
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
    *value = animate_blend_value(animatable.value.previous,
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
animate_from_to(float const a,
                float const b,
                double const time,
                double const duration)
{
    float const scalar = duration > SECONDS(0) ?
        (float)(time / duration) : 1;

    float d = animate_lerp(a, b, scalar);

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

    float scalar = (float)interpolation;

    if (scalar < 0) {
        scalar = 0;
    } else if (scalar > 1) {
        scalar = 1;
    }

    return animate_lerp(previous, next, scalar);
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
