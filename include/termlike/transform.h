#pragma once

#include <stdint.h> // int32_t

#define SCALE(x) (x)
#define ANGLE(x) (x)

enum term_rotate {
    /**
     * Apply rotation to the string as a whole.
     */
    TERM_ROTATE_STRING,
    /**
     * Apply rotation only to individual characters; not the string as a whole.
     */
    TERM_ROTATE_CHARACTERS
};

struct term_scale {
    /**
     * The amount to scale by horizontally (1 is no scaling).
     */
    float horizontal;
    /**
     * The amount to scale by vertically (1 is no scaling).
     */
    float vertical;
};

struct term_rotation {
    /**
     * The angle, in degrees, to apply rotation by (0 is no rotation).
     */
    int32_t angle;
    /**
     * The method of rotation to apply.
     */
    enum term_rotate rotation;
};

struct term_transform {
    /**
     * The scaling transformation.
     */
    struct term_scale scale;
    /**
     * The rotation transformation.
     */
    struct term_rotation rotate;
};

extern struct term_transform const TERM_TRANSFORM_NONE;

struct term_transform rotated(int32_t angle, enum term_rotate);
struct term_transform scaled(float scale);
struct term_transform transformed(float scale, int32_t angle, enum term_rotate);
