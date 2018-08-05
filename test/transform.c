#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

static int32_t a = 0;

static
void
draw(double const interp)
{
    (void)interp;
    
    int32_t cw, ch;
    
    term_measure("█", &cw, &ch);
    
    struct term_position position = positioned(10, 26);
        
    // case: different ways of achieving same transformation
    // expected: all boxes are aligned, same size and equally spaced
    term_printt(positioned_offset(position, 0,  0), TERM_COLOR_WHITE, scaled(1), "██\n██");
    term_printt(positioned_offset(position, cw * 3, 0), TERM_COLOR_WHITE, scaled(2), "█");
    // note that the fill should be identical to scaled(2), as above
    //  e.g. that a single glyph is scaled to fit area
    term_fillt(positioned_offset(position, cw * 6, 0), sized(2*cw, 2*ch), TERM_COLOR_WHITE, TERM_TRANSFORM_NONE);
    
    // case: boxes scaled and rotated
    // note how this one is rotated using default anchoring (center), to rotate in line with the following two
    // (this is because it is *not* a single glyph, and thus would default to rotate using first glyph as anchor)
    term_printt(positioned_offset(position, 100,  0), TERM_COLOR_WHITE, transformed(1, a, TERM_ROTATE_STRING_ANCHORED), "██\n██");
    // these automatically rotate around their center because they're represent only a single glyph
    term_printt(positioned_offset(position, 100 + cw * 3, 0), TERM_COLOR_WHITE, transformed(2, a, TERM_ROTATE_STRING), "█");
    term_fillt(positioned_offset(position, 100 + cw * 6, 0), sized(2*cw, 2*ch), TERM_COLOR_WHITE, rotated(a, TERM_ROTATE_STRING));
    
    // case: individual glyph rotation
    term_printstrt(positioned(10, 80), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, rotated(a, TERM_ROTATE_CHARACTERS), "•rotate");
    term_printstrt(positioned(10, 100), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, transformed(2, a, TERM_ROTATE_CHARACTERS), "•rotate");
    // case: rotation anchored around center of glyph
    term_printstrt(positioned(140, 80), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, rotated(a, TERM_ROTATE_STRING), "•rotate me");
    term_printstrt(positioned(140, 100), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, transformed(2, a, TERM_ROTATE_STRING), "•rotate me");
    
    
    term_printstr(positioned_offset(position, 0, -ch * 2), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, "PRINT");
    term_printstr(positioned_offset(position, cw * 6, -ch * 2), TERM_COLOR_WHITE, TERM_BOUNDS_NONE, "FILL");
    term_print(positioned_offset(position, cw * 2, 0), colored(0, 225, 0), "█");
    term_print(positioned_offset(position, cw * 5, 0), colored(0, 225, 0), "█");
    term_print(position, transparent(colored(255, 0, 0), 0.25f), "██████████████████████");
    term_print(position, colored(225, 0, 0),                     "──────────────────────");
    
    // case: word-wrap scaled text
    struct term_bounds bounds = bounded(cw * 8, ch * 12);
    
    term_fill(positioned(10, 136), bounds.size, colored(255, 255, 225));
    term_printstrt(positioned(10, 136), colored(255, 50, 50), bounds, scaled(2), "wrap me inside filled area");
}

int32_t
main(void)
{
    struct term_settings settings = defaults("Termlike: Transformation test");
    
    settings.pixel_size = 2;
    
    if (!term_open(settings)) {
        exit(EXIT_FAILURE);
    }
    
    term_set_drawing(draw);
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        if (term_key_down(TERM_KEY_UP)) {
            a += 1;
        } else if (term_key_down(TERM_KEY_DOWN)) {
            a -= 1;
        }
        
        if (a > 360 || a < -360) {
            a = 0;
        }
        
        term_run(TERM_FREQUENCY_NONE);
    }
    
    term_close();
    
    return 0;
}
