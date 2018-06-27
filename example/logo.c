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
    
    char const * const logo =
    "___ ____ ____ _  _ _    _ _  _ ____\n"
    " |  |___ |__/ |\\/| |    | |_/  |___\n"
    " |  |___ |  \\ |  | |___ | | \\_ |___";
    
    int32_t cw, ch;
    
    term_measure(logo, &cw, &ch);
    
    int32_t x = (w / 2) - (cw / 2);
    int32_t y = (h / 2) - (ch / 2) - 40;
    
    term_print(positionedz(x, y, layered(1)),
               colored(255, 255, 255),
               logo);

    float alpha = 0.2f;
    
    for (int32_t i = 1; i < 8; i++) {
        term_printt(positionedz(x, y, layered(0)),
                    transparent(255, 255, 255, alpha),
                    transformed(1, -i, TERM_ROTATE_STRING),
                    logo);
        
        alpha *= 0.8f;
    }

    term_printstr(positioned((w / 2), h - 60),
                  colored(255, 255, 255),
                  align(bounded((w / 2) + (w / 4), h / 2), TERM_ALIGN_CENTER),
                  "POWERED BY CP437");
}

int32_t
main(void)
{
    if (!term_open(defaults(""))) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw_logo);
    
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
