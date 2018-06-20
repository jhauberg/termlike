#pragma once

#include <stdint.h> // int32_t


struct term_location {
    int32_t x, y;
};

struct term_location located(int32_t x, int32_t y);
