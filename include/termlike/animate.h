#pragma once

#include <stdint.h> // int32_t

#define SECONDS(s) (s)
#define MILLISECONDS(ms) (ms / 1000.0)

struct term_frame {
    float original;
    float current;
    float previous;
};

struct term_animate {
    struct term_frame value;
    /**
     * The accumulated amount of time passed during animation.
     */
    double time;
};

inline
struct term_animate
animated(float const value)
{
    return (struct term_animate) {
        .value = {
            .original = value,
            .current = value,
            .previous = value
        },
        .time = SECONDS(0)
    };
}

void animate_by(struct term_animate *, float value, double s);
void animate_to(struct term_animate *, float value, double duration, double s);

void animate_blend(struct term_animate, double interp, int32_t * value);
void animate_blendf(struct term_animate, double interp, float * value);
