#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

static
void
draw_logo(double const interp)
{
    (void)interp;
    
    struct term_dimens display;
    
    term_get_display(&display);
    
    char const * const map =
    "       ░\n"
    "       ░\n"
    " ┌─────∙┐\n"
    "░∙∙∙∙∙∙∙│\n"
    " │∙∙@∙∙∙│\n"
    " │∙∙∙∙∙\n"
    " │∙∙";
    
    struct term_dimens map_size;

    struct term_bounds const bounds = bounded(display.width,
                                              display.height);

    term_measurestr(map, bounds, &map_size);

    term_printstr(map,
                  positioned((display.width / 2) - (map_size.width / 2),
                             (display.height / 2) - (map_size.height / 2)),
                  colored(255, 255, 255),
                  bounds);
}

int32_t
main(void)
{
    if (!term_open(defaults(""))) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw_logo);
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_run(TERM_FREQUENCY_NONE);
    }
    
    term_close();
    
    return 0;
}
