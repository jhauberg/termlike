#pragma once

#include <stdint.h> // int32_t

#include "layer.h" // term_layer :completeness

struct term_location {
    int32_t x;
    int32_t y;
};

struct term_position {
    struct term_location location;
    struct term_layer layer;
};

struct term_position positioned(int32_t x, int32_t y);
struct term_position positionedz(int32_t x, int32_t y, struct term_layer);
