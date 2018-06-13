#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool

static
void
draw(double const interp)
{
    (void)interp;
}

static
void
tick(double const step)
{
    (void)step;
}

int32_t
main(void)
{
    if (!term_open(term_defaults("Termlike"))) {
        exit(EXIT_FAILURE);
    }

    term_set_drawing(draw);
    term_set_ticking(tick);
    
    while (!term_is_closing()) {
        // note that this key check is not done in tick()
        // input can be "lost" if there's too long between ticks
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_run(TERM_FREQUENCY_DEFAULT);
    }
    
    term_close();
    
    return 0;
}
