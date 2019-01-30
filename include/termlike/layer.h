#pragma once

#include <stdint.h> // uint8_t

/**
 * Represents a layer.
 *
 * Layers are used to indicate the order that glyphs and strings should be
 * drawn in, where glyphs on a lower layer are drawn before glyphs on a
 * higher layer.
 *
 * If two glyphs would be drawn on the same layer and at the same depth, then
 * the latter issued glyph is drawn after the former (i.e. on top).
 *
 * There are a maximum of 255 layers, with each layer offering additional
 * 255 sub-layers (depth).
 */
struct term_layer {
    /**
     * The sub-layer order.
     */
    uint8_t depth;
    /**
     * The layer order.
     */
    uint8_t index;
};

/**
 * The highest layer.
 */
extern struct term_layer const TERM_LAYER_TOP;
/**
 * The lowest layer.
 */
extern struct term_layer const TERM_LAYER_BOTTOM;

/**
 * The default layer (bottom).
 */
#define TERM_LAYER_DEFAULT (TERM_LAYER_BOTTOM)

/**
 * Return the layer at a certain depth.
 */
inline
struct term_layer
layered_depth(uint8_t const index, uint8_t const depth)
{
    return (struct term_layer) {
        .depth = depth,
        .index = index
    };
}

/**
 * Return the layer at default depth.
 */
inline
struct term_layer
layered(uint8_t const index)
{
    return layered_depth(index, TERM_LAYER_DEFAULT.depth);
}

/**
 * Return the layer directly below a layer, if any. Same layer otherwise.
 *
 * If there is a sub-layer below the layer, then that layer is returned.
 */
struct term_layer layered_below(struct term_layer);
/**
 * Return the layer directly above a layer, if any. Same layer otherwise.
 *
 * If there is a sub-layer above the layer, then that layer is returned.
 */
struct term_layer layered_above(struct term_layer);
