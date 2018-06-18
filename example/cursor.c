#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;
    
    struct term_cursor_state const cursor = term_cursor();
    
    term_print(cursor.location.x - 8,
               cursor.location.y - 8,
               TERM_COLOR_WHITE,
               layered(1),
               "█");
}

int32_t
main(void)
{
    if (!term_open(term_defaults("Termlike: Cursor"))) {
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
