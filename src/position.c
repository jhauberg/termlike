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

struct term_position
positioned_offset(struct term_position position,
                  int32_t const dx,
                  int32_t const dy)
{
    position.location.x += dx;
    position.location.y += dy;
    
    return position;
}

struct term_position
positioned_layer(struct term_position position, struct term_layer const layer)
{
    position.layer = layer;
    
    return position;
}
