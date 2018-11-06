#include <termlike/position.h> // term_position, positioned*

#include <stdint.h> // int32_t

extern inline struct term_position positioned(int32_t x, int32_t y);
extern inline struct term_position positionedz(int32_t x, int32_t y,
                                               struct term_layer);
