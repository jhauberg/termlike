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
    
    term_cursor(&cursor);
    
    int32_t w, h;
    
    term_measure(pointer, &w, &h);
    term_print(located(cursor.location.x - (w / 2),
                       cursor.location.y - (h / 2)),
               TERM_COLOR_WHITE,
               layered(1),
               pointer);
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
        
        term_run(TERM_FREQUENCY_ONCE_A_SECOND);
    }
    
    term_close();
    
    return 0;
}
