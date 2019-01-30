#pragma once

#include <stdint.h> // int32_t

#include "layer.h" // term_layer :completeness

/**
 * Represents a pixel coordinate.
 */
struct term_location {
    /**
     * The coordinate point on the horizontal axis.
     */
    int32_t x;
    /**
     * The coordinate point on the vertical axis.
     */
    int32_t y;
};

/**
 * Represents a layered pixel coordinate.
 */
struct term_position {
    /**
     * The pixel coordinate.
     */
    struct term_location location;
    /**
     * The depth coordinate.
     */
    struct term_layer layer;
};

/**
 * Return the position at a layered pixel coordinate.
 */
inline
struct term_position
positionedz(int32_t const x,
            int32_t const y,
            struct term_layer const layer)
{
    return (struct term_position) {
        .location = {
            .x = x,
            .y = y
        },
        .layer = layer
    };
}

/**
 * Return the position at a pixel coordinate.
 *
 * The position is layered at the default layer (bottom).
 */
inline
struct term_position
positioned(int32_t const x,
           int32_t const y)
{
    return positionedz(x, y, TERM_LAYER_DEFAULT);
}
