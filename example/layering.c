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
    
    struct term_dimens c;
    
    term_measure("█", &c);
    
    int32_t x = 0;
    int32_t y = 0;
    
    int32_t const p = 2;
    
    // case: correct layering and order
    term_print(bottom, positionedz(x, y, layered(0)), bottom_color);
    term_print(top, positionedz(x + p, y + p, layered(1)), top_color);
    
    x += c.width * 2;
    
    // case: correct layering but reverse order
    term_print(top, positionedz(x + p, y + p, layered(1)), top_color);
    term_print(bottom, positionedz(x, y, layered(0)), bottom_color);
    
    x += c.width * 2;
    
    // case: same layer but correct order
    term_print(bottom, positioned(x, y), bottom_color);
    term_print(top, positioned(x + p, y + p), top_color);
    
    x += c.width * 2;
    
    // case: same layer but reverse order
    // expected: bottom on top
    term_print(top, positioned(x + p, y + p), top_color);
    term_print(bottom, positioned(x, y), bottom_color);
    
    x += c.width * 2;
    
    // case: same layer, correct ordering, but one is transformed
    term_set_transform(rotated(45, TERM_ROTATE_STRING));
    term_print(bottom, positionedz(x, y, layered(0)), bottom_color);
    term_set_transform(TERM_TRANSFORM_NONE);
    term_print(top, positionedz(x + p, y + p, layered(0)), top_color);
    
    x += c.width * 2;
    
    // case: same layer, correct ordering, but one is transformed
    term_print(bottom, positionedz(x, y, layered(0)), bottom_color);
    term_set_transform(rotated(45, TERM_ROTATE_STRING));
    term_print(top, positionedz(x + p, y + p, layered(0)), top_color);
    term_set_transform(TERM_TRANSFORM_NONE);
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
