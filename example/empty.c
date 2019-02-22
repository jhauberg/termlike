#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

static
void
tick(double const step)
{
    (void)step;
}

static
void
draw(double const interp)
{
    (void)interp;
}

int32_t
main(void)
{
    struct term_settings settings = defaults("Termlike: Empty");

    settings.vsync = false;

    if (!term_open(settings)) {
        exit(EXIT_FAILURE);
    }

    term_set_ticking(tick);
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
