#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;
    
    char const * const background = "█";

    int32_t w, h;
    
    term_get_display(&w, &h);
    
    int32_t cw, ch;
    
    term_measure(background, &cw, &ch);
    
    int32_t const columns = w / cw;
    int32_t const rows = h / ch;
    
    struct term_layer background_layer = layered(0);
    struct term_layer foreground_layer = layered_above(background_layer);
    
    for (uint16_t column = 0; column < columns; column++) {
        for (uint16_t row = 0; row < rows; row++) {
            int32_t x = column * cw;
            int32_t y = row * ch;
            
            struct term_color background_color = colored(rand() % 255,
                                                         rand() % 255,
                                                         rand() % 255);
            
            term_print(positionedz(x, y, background_layer),
                       background_color,
                       background);
            
            struct term_color foreground_color = colored(rand() % 255,
                                                         rand() % 255,
                                                         rand() % 255);
            
            term_print(positionedz(x, y, foreground_layer),
                       foreground_color,
                       "A");
            
            foreground_color.r = rand() % 255;
            
            term_printt(positionedz(x, y, layered_above(foreground_layer)),
                        transparent(foreground_color, (rand() % 100) / 100.0f),
                        rotated(rand() % 360, TERM_ROTATE_CHARACTERS),
                        "B");
        }
    }
}

int32_t
main(void)
{
    struct term_settings settings = defaults("Termlike: Performance test");
    
    settings.vsync = false;
    settings.pixel_size = 1;
    
    if (!term_open(settings)) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw);
    
    srand(1234);
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_run(TERM_FREQUENCY_NONE);
    }
    
    term_close();
    
    return 0;
}
