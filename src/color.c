#include <termlike/color.h> // term_color, colored*

#include <stdint.h> // uint8_t

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
colored(uint8_t const red,
      uint8_t const green,
      uint8_t const blue)
{
    return colored_transparency(red, green, blue, 1);
}

struct term_color
colored_transparency(uint8_t const red,
                     uint8_t const green,
                     uint8_t const blue,
                     float const alpha)
{
    return (struct term_color) {
        .r = red,
        .g = green,
        .b = blue,
        .a = alpha
    };
}
