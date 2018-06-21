#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;
    
    term_print(positioned(2, 2), TERM_COLOR_WHITE, "Hello.");
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
