#include <termlike/layer.h> // term_layer, layered*

#include <stdint.h> // uint8_t, UINT8_MAX

#include "internal.h" // layer_z

#define LAYER_MIN_INDEX 0
#define LAYER_MAX_INDEX UINT8_MAX

#define LAYER_MIN_DEPTH 0
#define LAYER_MAX_DEPTH UINT8_MAX

struct term_layer const TERM_LAYER_TOP = {
    .depth = LAYER_MAX_DEPTH,
    .index = LAYER_MAX_INDEX
};

struct term_layer const TERM_LAYER_BOTTOM = {
    .depth = LAYER_MIN_DEPTH,
    .index = LAYER_MIN_INDEX
};

#define LAYER_MAX_INDEX_PLUS_ONE (LAYER_MAX_INDEX + 1)

static inline float layer_z_between(float value, float min, float max);
static inline float layer_lerp(float a, float b, float t);

extern inline struct term_layer layered(uint8_t index);
extern inline struct term_layer layered_depth(uint8_t index, uint8_t depth);

struct term_layer
layered_below(struct term_layer const layer)
{
    struct term_layer layer_below = layer;
    
    if (layer.depth > LAYER_MIN_DEPTH) {
        layer_below.depth = layer.depth - 1;
    } else if (layer.index > LAYER_MIN_INDEX) {
        layer_below.index = layer.index - 1;
    }
    
    return layer_below;
}

struct term_layer
layered_above(struct term_layer const layer)
{
    struct term_layer layer_above = layer;
    
    if (layer.depth < LAYER_MAX_DEPTH) {
        layer_above.depth = layer.depth + 1;
    } else if (layer.index < LAYER_MAX_INDEX) {
        layer_above.index = layer.index + 1;
    }
    
    return layer_above;
}

float
layer_z(struct term_layer const layer)
{
    // to provide a depth range for the top-most layer (e.g. 255),
    // we add an additional layer just above
    float const z = layer_z_between(layer.index,
                                    LAYER_MIN_INDEX,
                                    LAYER_MAX_INDEX_PLUS_ONE);
    
    float const z_above = layer_z_between(layer.index + 1,
                                          LAYER_MIN_INDEX,
                                          LAYER_MAX_INDEX_PLUS_ONE);
    
    float const depth_z = layer_z_between(layer.depth,
                                          LAYER_MIN_DEPTH,
                                          LAYER_MAX_DEPTH);
    
    return layer_lerp(z, z_above, depth_z);
}

static
inline
float
layer_lerp(float const a, float const b, float const t)
{
    return (1.0f - t) * a + t * b;
}

static
inline
float
layer_z_between(float const value, float const min, float const max)
{
    return (value - min) / (max - min);
}
