#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;

    char const * const bottom = "█";
    char const * const top = "▓";
    
    struct term_color const bottom_color = colored(255, 0, 0);
    struct term_color const top_color = colored(255, 255, 255);
    
    int32_t w, h;
    
    term_measure("█", &w, &h);
    
    int32_t x = 0;
    int32_t y = 0;
    
    int32_t const p = 2;
    
    // case: correct layering and order
    term_print(positionedz(x, y, layered(0)), bottom_color, bottom);
    term_print(positionedz(x + p, y + p, layered(1)), top_color, top);
    
    x += w * 2;
    
    // case: correct layering but reverse order
    term_print(positionedz(x + p, y + p, layered(1)), top_color, top);
    term_print(positionedz(x, y, layered(0)), bottom_color, bottom);
    
    x += w * 2;
    
    // case: same layer but correct order
    term_print(positioned(x, y), bottom_color, bottom);
    term_print(positioned(x + p, y + p), top_color, top);
    
    x += w * 2;
    
    // case: same layer but reverse order
    // expected: bottom on top
    term_print(positioned(x + p, y + p), top_color, top);
    term_print(positioned(x, y), bottom_color, bottom);
}

int32_t
main(void)
{
    struct term_settings settings = defaults("Termlike: Layering test");
    
    settings.pixel_size = 2;
    
    if (!term_open(settings)) {
        exit(EXIT_FAILURE);
    }
    
    // note that this test requires each glyph to be flushed immediately
    // (e.g. set MAX_GLYPHS to 1)
    term_set_drawing(draw);
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_run(TERM_FREQUENCY_ONCE_A_SECOND);
    }
    
    term_close();
    
    return 0;
}
