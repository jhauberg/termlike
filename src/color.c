#include <termlike/color.h> // term_color, colored*

#include <stdint.h> // uint8_t, int32_t

struct term_color const TERM_COLOR_WHITE = {
    .a = 1,
    .r = 255,
    .g = 255,
    .b = 255
};

struct term_color const TERM_COLOR_BLACK = {
    .a = 1,
    .r = 0,
    .g = 0,
    .b = 0
};

struct term_color
coloredh(int32_t const hex)
{
    return colored((hex & 0xFF0000) >> 16,
                   (hex & 0xFF00) >> 8,
                   (hex & 0xFF) >> 0);
}

struct term_color
colored(uint8_t const red, uint8_t const green, uint8_t const blue)
{
    return (struct term_color) {
        .r = red,
        .g = green,
        .b = blue,
        .a = 1
    };
}

struct term_color
transparent(struct term_color color, float const alpha)
{
    color.a = alpha;
    
    return color;
}
