#include <termlike/position.h> // term_position, positioned*

extern inline struct term_position positioned(int32_t x, int32_t y);
extern inline struct term_position positionedz(int32_t x, int32_t y,
                                               struct term_layer);

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
