#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

// note: interesting and simple load mechanism by rotating "•"
//       it deforms into 3 different rasterizations
// note: similarly, rotating a block glyph just slightly, results
//       in rounded corners

// idea: pachinko machine
//      - pegs are bullet+outer bullet
//        collision is sphere X sphere, and reflect angle can be 360 degrees

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
    struct term_settings settings = defaults("golf");
    
    settings.size = TERM_SIZE_640W;
    settings.vsync = true;
    
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
