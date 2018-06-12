#include "loader.h" // load_*, image_loaded_callback

#include "renderer.h" // graphics_image

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wunused-function"
 #pragma clang diagnostic ignored "-Wunused-parameter"
 #pragma clang diagnostic ignored "-Wsign-conversion"
 #pragma clang diagnostic ignored "-Wconversion"
 #pragma clang diagnostic ignored "-Wpadded"
 #pragma clang diagnostic ignored "-Wcomma"
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG

#include <stb/stb_image.h> // stbi_*

#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

bool
load_image_data(uint8_t const * const buffer,
                int32_t const length,
                image_loaded_callback * const callback)
{
    stbi_set_flip_vertically_on_load(true);
    
    struct graphics_image image;
    
    image.data = stbi_load_from_memory(buffer,
                                       length,
                                       &image.width, &image.height,
                                       &image.components,
                                       STBI_rgb_alpha);
    
    if (!image.data) {
        return false;
    }
    
    if (callback) {
        callback(image);
    }
    
    stbi_image_free(image.data);
    
    return true;
}
