#pragma once

#include <stdbool.h> // bool
#include <stdint.h> // uint8_t, int32_t

struct graphics_image;

typedef void image_loaded_callback(struct graphics_image);

bool load_image_data(uint8_t const * buffer,
                     int32_t length,
                     image_loaded_callback *);
