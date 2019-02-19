#include <termlike/transform.h> // term_transform, term_anchor

#include "internal.h" // rotate_point

#include <stdint.h> // int32_t
#include <math.h> // sinf, cosf

struct term_transform const TERM_TRANSFORM_NONE = {
    .scale = {
        .horizontal = SCALE(1),
        .vertical = SCALE(1)
    },
    .rotate = {
        .angle = ANGLE(0),
        .rotation = TERM_ROTATE_STRING_ANCHORED,
        .anchor = {
            .x = 0.5f, // center
            .y = 0.5f // center
        }
    }
};

extern inline struct term_transform rotated(int32_t angle,
                                            enum term_rotate);
extern inline struct term_transform scaled(float scale);
extern inline struct term_transform transformed(float scale,
                                                int32_t angle,
                                                enum term_rotate);

void
rotate_point(struct term_anchor const point, float const angle,
             struct term_anchor * const result)
{
    struct term_anchor const center = {
        .x = 0,
        .y = 0
    };

    rotate_point_center(point, center, angle, result);
}

void
rotate_point_center(struct term_anchor const point,
                    struct term_anchor const center, float const angle,
                    struct term_anchor * const result)
{
    float const s = sinf(angle);
    float const t = cosf(angle);

    float const x = point.x - center.x;
    float const y = point.y - center.y;

    result->x = ((x * t) - (y * s)) + center.x;
    result->y = ((x * s) + (y * t)) + center.y;
}
