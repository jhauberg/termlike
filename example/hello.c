#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;
    
    // determine display resolution
    struct term_dimens display;
    
    term_get_display(&display);
    
    // measure to determine dimensions of any single character
    struct term_dimens c;
    
    term_measure(TERM_SINGLE_GLYPH, &c);
    
    // print a long text, padded to the border of the display
    char const * const message =
    "Hello.\n\n"
    "This example program shows you how to print regular text (like this).\n\n"
    "Long texts are wrapped when reaching boundaries (if any), either by "
    "words or individual characters.\n\n"
    "Any text that exceeds the internal buffer will be cut short.\n\n"
    "Like thiiis right here!";
    
    // define boundaries so the text will wrap inside the display
    int32_t pad_x = c.width / 2;
    int32_t pad_y = c.height / 2;
    
    struct term_bounds bounds = bounded(display.width - (pad_x * 2),
                                        (display.height / 2) - (pad_y * 2));
    
    // words exceeding boundaries will be wrapped to the next line
    bounds.wrap = TERM_WRAP_WORDS;
    
    term_printstr(message,
                  positioned(pad_x, pad_y),
                  colored(255, 255, 225),
                  bounds);
    
    // measure size of the printed text
    struct term_dimens measured;
    
    term_measurestr(message, &measured, bounds);
    
    // characters exceeding boundaries will be wrapped to the next line
    bounds.wrap = TERM_WRAP_CHARACTERS;
    
    term_printstr(message,
                  positioned(pad_x, pad_y + measured.height + c.height * 2),
                  colored(255, 255, 245),
                  bounds);
}

int32_t
main(void)
{
    if (!term_open(defaults("Termlike"))) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw);

    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }

        term_run(TERM_FREQUENCY_NONE);
    }

    term_close();
    
    return 0;
}
