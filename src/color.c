#include <termlike/color.h> // term_color, colored*

#include <stdint.h> // uint8_t, int32_t

struct term_color const TERM_COLOR_WHITE = {
    .a = 255,
    .r = 255,
    .g = 255,
    .b = 255
};

struct term_color const TERM_COLOR_BLACK = {
    .a = 255,
    .r = 0,
    .g = 0,
    .b = 0
};

extern inline struct term_color colored(uint8_t red,
                                        uint8_t green,
                                        uint8_t blue);
extern inline struct term_color coloredh(int32_t hex);

extern inline struct term_color transparent(struct term_color color,
                                            uint8_t alpha);
