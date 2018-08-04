#include <termlike/transform.h> // term_transform

#include "internal.h" // rotate_point

#include <stdint.h> // int32_t

#include <linmath/linmath.h> // vec2

struct term_transform const TERM_TRANSFORM_NONE = {
    .scale = {
        .horizontal = SCALE(1),
        .vertical = SCALE(1)
    },
    .rotate = {
        .angle = ANGLE(0),
        .rotation = TERM_ROTATE_STRING,
        .anchor = {
            .x = 0,
            .y = 0
        }
    }
};

struct term_transform
rotated(int32_t const angle, enum term_rotate const rotation)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.rotate.angle = angle;
    transform.rotate.rotation = rotation;
    
    return transform;
}

struct term_transform
scaled(float const scale)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.scale.horizontal = scale;
    transform.scale.vertical = scale;
    
    return transform;
}

struct term_transform
transformed(float const scale,
            int32_t const angle,
            enum term_rotate const rotation)
{
    return (struct term_transform) {
        .scale = {
            .horizontal = scale,
            .vertical = scale,
        },
        .rotate = {
            .angle = angle,
            .rotation = rotation,
            .anchor = {
                .x = 0.5f, // center
                .y = 0.5f // center
            }
        }
    };
}

void
rotate_point(vec2 p, float const angle, vec2 dst)
{
    vec2 center = { 0, 0 };
    
    rotate_point_center(p, center, angle, dst);
}

void
rotate_point_center(vec2 p, vec2 c, float const angle, vec2 dst)
{
    float const s = sinf(angle);
    float const t = cosf(angle);

    float const x = p[0] - c[0];
    float const y = p[1] - c[1];
    
    dst[0] = ((x * t) - (y * s)) + c[0];
    dst[1] = ((x * s) + (y * t)) + c[1];
}
