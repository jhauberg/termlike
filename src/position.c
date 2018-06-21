#include <termlike/position.h> // term_position, positioned*

struct term_position
positioned(int32_t const x, int32_t const y)
{
    return (struct term_position) {
        .location = {
            .x = x,
            .y = y
        },
        .layer = TERM_LAYER_DEFAULT
    };
}

struct term_position
positionedz(int32_t const x, int32_t const y, struct term_layer layer)
{
    return (struct term_position) {
        .location = {
            .x = x,
            .y = y
        },
        .layer = layer
    };
}
