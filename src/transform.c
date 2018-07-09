#include <termlike/transform.h> // term_transform

#include <termlike/position.h> // term_location

#include "internal.h" // rotate_point

#include <stdint.h> // int32_t

#include <linmath/linmath.h> // mat4x4

struct term_transform const TERM_TRANSFORM_NONE = {
    .scale = {
        .horizontal = SCALE(1),
        .vertical = SCALE(1)
    },
    .rotate = {
        .angle = ANGLE(0),
        .rotation = TERM_ROTATE_STRING
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
            .rotation = rotation
        }
    };
}

void
rotate_point(struct term_location const point,
             struct term_location const origin,
             float angle,
             struct term_location * const location)
{
    mat4x4 translated;
    mat4x4_translate(translated, origin.x, origin.y, 0);
    
    mat4x4 rotated;
    mat4x4_identity(rotated);
    mat4x4_rotate_Z(rotated, rotated, angle);
    
    mat4x4 transform;
    mat4x4_mul(transform, translated, rotated);
    
    vec4 position = {
        point.x,
        point.y,
        0, 1
    };
    
    vec4 transformed_point;
    mat4x4_mul_vec4(transformed_point, transform, position);
    
    location->x = (int32_t)transformed_point[0];
    location->y = (int32_t)transformed_point[1];
}
