#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;
    
    char const * const pointer =
        term_key_down(TERM_KEY_MOUSE_LEFT) ? "▓" : "█";
    
    struct term_cursor_state cursor;
    
    term_get_cursor(&cursor);
    
    struct term_dimens c;

    term_measure(pointer, &c);
    term_print(pointer, positionedz(cursor.location.x - (c.width / 2),
                                    cursor.location.y - (c.height / 2),
                                    layered(1)),
               TERM_COLOR_WHITE);
}

int32_t
main(void)
{
    if (!term_open(defaults("Termlike: Cursor"))) {
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
