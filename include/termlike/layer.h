#pragma once

#include <stdint.h> // uint8_t

struct term_layer {
    uint8_t depth;
    uint8_t index;
};

extern struct term_layer const TERM_LAYER_TOP;
extern struct term_layer const TERM_LAYER_BOTTOM;

#define TERM_LAYER_DEFAULT (TERM_LAYER_BOTTOM)

inline
struct term_layer
layered_depth(uint8_t const index, uint8_t const depth)
{
    return (struct term_layer) {
        .depth = depth,
        .index = index
    };
}

inline
struct term_layer
layered(uint8_t const index)
{
    return layered_depth(index, TERM_LAYER_DEFAULT.depth);
}
    
struct term_layer layered_below(struct term_layer);
struct term_layer layered_above(struct term_layer);
