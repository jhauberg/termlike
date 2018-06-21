#include <termlike/transform.h> // term_transform

#include <stdint.h> // int32_t

struct term_transform const TERM_TRANSFORM_NONE = {
    .scale = SCALE(1),
    .angle = ANGLE(0),
    .rotation = TERM_ROTATE_STRING
};

struct term_transform
rotated(int32_t const angle,
        enum term_rotate const rotation)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.angle = angle;
    transform.rotation = rotation;
    
    return transform;
}

struct term_transform
scaled(float const scale)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.scale = scale;
    
    return transform;
}

struct term_transform
transformed(float const scale,
            int32_t const angle,
            enum term_rotate const rotation)
{
    return (struct term_transform) {
        .scale = scale,
        .angle = angle,
        .rotation = rotation
    };
}
