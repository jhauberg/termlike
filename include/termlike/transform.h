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
     * Apply rotation anchored relatively to the string as a whole.
     */
    TERM_ROTATE_STRING_ANCHORED,
    /**
     * Apply rotation to each individual glyph; not the string as a whole.
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

struct term_anchor {
    float x;
    float y;
};

struct term_rotation {
    /**
     * The relative point at which rotation is applied.
     *
     * An anchor point is defined by a floating point coordinate, as per the
     * diagram visualized below:
     *
     *    (0,0)───(.5,0)───(1,0)
     *      │        │       │
     *      │        │       │
     *    (0,.5)──(.5,.5)──(1,.5)
     *      │        │       │
     *      │        │       │
     *    (0,1)───(.5,1)───(1,1)
     *
     * Points can range anywhere between, or even beyond, the listed values.
     */
    struct term_anchor anchor;
    /**
     * The method of rotation to apply.
     */
    enum term_rotate rotation;
    /**
     * The angle, in degrees, to apply rotation by (0 is no rotation).
     */
    int32_t angle;
};

struct term_transform {
    /**
     * The scaling transformation.
     *
     * Scaling is applied from the top-left corner of each glyph, making it
     * stretch down- and rightward.
     */
    struct term_scale scale;
    /**
     * The rotation transformation.
     *
     * Rotation is applied to the center of each glyph. This is the case no
     * matter the specified rotation mode. However, when rendering strings of
     * glyphs there are some specific behaviors that come into effect:
     *
     * 1) When applying rotation to a string of glyphs (using
     * `TERM_ROTATE_STRING`), then each glyph following the first is rotated
     * additionally to appear as a whole.
     *
     * This effectively means that the first glyph in a string becomes the
     * anchor point for the string as a whole, rotating each following glyph
     * around that point.
     *
     * 2) When specifying an anchor point on a string of glyphs (e.g. using
     * `TERM_ROTATE_STRING_ANCHORED`), then each glyph is rotated additionally
     * around the defined anchor.
     */
    struct term_rotation rotate;
};

extern struct term_transform const TERM_TRANSFORM_NONE;

inline
struct term_transform
rotated(int32_t const angle,
        enum term_rotate const rotation)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.rotate.angle = angle;
    transform.rotate.rotation = rotation;
    
    return transform;
}

inline
struct term_transform
scaled(float const scale)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.scale.horizontal = scale;
    transform.scale.vertical = scale;
    
    return transform;
}

inline
struct term_transform
transformed(float const scale,
            int32_t const angle,
            enum term_rotate const rotation)
{
    struct term_transform transform = TERM_TRANSFORM_NONE;
    
    transform.scale.horizontal = scale;
    transform.scale.vertical = scale;
    transform.rotate.angle = angle;
    transform.rotate.rotation = rotation;
    
    return transform;
}
