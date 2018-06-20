#include <termlike/location.h> // term_location

struct term_location
located(int32_t const x, int32_t const y)
{
    return (struct term_location) {
        .x = x,
        .y = y
    };
}
