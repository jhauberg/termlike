#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

static
void
draw_logo(double const interp)
{
    (void)interp;
    
    int32_t w, h;
    
    term_get_display(&w, &h);
    
    char const * const map =
    "       ░\n"
    "       ░\n"
    " ┌─────∙┐\n"
    "░∙∙∙∙∙∙∙│\n"
    " │∙∙@∙∙∙│\n"
    " │∙∙∙∙∙\n"
    " │∙∙";
    
    int32_t mw, mh;
    
    term_measure(map, &mw, &mh);
    
    term_print(positioned((w / 2) - (mw / 2),
                          (h / 2) - (mh / 2)),
               colored(255, 255, 255),
               map);
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
