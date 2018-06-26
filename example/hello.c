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
    int32_t w, h;
    
    term_get_display(&w, &h);
    
    // measure to determine dimensions of any single character
    int32_t cw, ch;
    
    term_measure("A", &cw, &ch);
    
    // print a long text, padded to the border of the display
    char const * const message =
    "Hello.\n\n"
    "This example program shows you how to print regular text (like this).\n\n"
    "Long texts are wrapped when reaching boundaries (if any), either by "
    "words or individual characters.\n\n"
    "Any text that exceeds the internal buffer will be cut short.\n\n"
    "Like thiiis right here!";
    
    // define boundaries so the text will wrap inside the display
    struct term_bounds bounds;
    
    int32_t pad_x = cw / 2;
    int32_t pad_y = ch / 2;
    
    bounds = bounded(w - (pad_x * 2),
                     (h / 2) - (pad_y * 2));
    
    // characters exceeding boundaries will be wrapped to the next line
    bounds.wrap = TERM_WRAP_CHARACTERS;
    
    term_printstr(positioned(pad_x, pad_y),
                  colored(255, 255, 255),
                  bounds,
                  message);

    // measure size of the printed text
    int32_t tw, th;
    
    term_measurestr(message, bounds, &tw, &th);
    
    // words exceeding boundaries will be wrapped to the next line
    bounds.wrap = TERM_WRAP_WORDS;
    
    term_printstr(positioned(pad_x, pad_y + th + ch * 2),
                  colored(255, 255, 225),
                  bounds,
                  message);
}

int32_t
main(void)
{
    if (!term_open(defaults("Termlike"))) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw);

    while (!term_is_closing()) {
        // note that this key check is not done in tick()
        // input can be "lost" if there's too long between ticks
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }

        term_run(TERM_FREQUENCY_ONCE_A_SECOND);
    }

    term_close();
    
    return 0;
}
