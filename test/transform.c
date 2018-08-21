#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t

static int32_t a = 0;

static
void
draw(double const interp)
{
    (void)interp;
    
    struct term_dimens c;
    
    term_measure("█", &c);
    
    struct term_position position = positioned(10, 26);
        
    // case: different ways of achieving same transformation
    // expected: all boxes are aligned, same size and equally spaced
    term_set_transform(scaled(1));
    term_print("██\n██", positioned_offset(position, 0,  0), TERM_COLOR_WHITE);
    term_set_transform(scaled(2));
    term_print("█", positioned_offset(position, c.width * 3, 0), TERM_COLOR_WHITE);
    // note that the fill should be identical to scaled(2), as above
    //  e.g. that a single glyph is scaled to fit area
    term_set_transform(TERM_TRANSFORM_NONE);
    term_fill(positioned_offset(position, c.width * 6, 0), sized(2 * c.width, 2 * c.height), TERM_COLOR_WHITE);
    
    // case: boxes scaled and rotated
    // note how this one is rotated using default anchoring (center), to rotate in line with the following two
    // (this is because it is *not* a single glyph, and thus would default to rotate using first glyph as anchor)
    term_set_transform(transformed(1, a, TERM_ROTATE_STRING_ANCHORED));
    term_print("██\n██", positioned_offset(position, 100,  0), TERM_COLOR_WHITE);
    // these automatically rotate around their center because they're represent only a single glyph
    term_set_transform(transformed(2, a, TERM_ROTATE_STRING));
    term_print("█", positioned_offset(position, 100 + c.width * 3, 0), TERM_COLOR_WHITE);
    term_set_transform(rotated(a, TERM_ROTATE_STRING));
    term_fill(positioned_offset(position, 100 + c.width * 6, 0), sized(2 * c.width, 2 * c.height), TERM_COLOR_WHITE);
    
    // case: anchored fill
    struct term_transform t = rotated(a, TERM_ROTATE_STRING_ANCHORED);
    
    t.rotate.anchor.x = 0;
    t.rotate.anchor.y = 0;
    
    term_set_transform(t);
    term_fill(positioned_offset(position, 100 + c.width * 9, 0), sized(2 * c.width, 2 * c.height), TERM_COLOR_WHITE);
    
    term_set_transform(rotated(a, TERM_ROTATE_CHARACTERS));
    // case: individual glyph rotation
    term_printstr("•rotate", positioned(10, 80), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    term_set_transform(transformed(2, a, TERM_ROTATE_CHARACTERS));
    term_printstr("•rotate", positioned(10, 100), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    // case: rotation anchored around center of glyph
    term_set_transform(rotated(a, TERM_ROTATE_STRING));
    term_printstr("•rotate me", positioned(140, 80), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    term_set_transform(transformed(2, a, TERM_ROTATE_STRING));
    term_printstr("•rotate me", positioned(140, 100), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    
    
    term_set_transform(TERM_TRANSFORM_NONE);
    term_printstr("PRINT", positioned_offset(position, 0, -c.height * 2), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    term_printstr("FILL", positioned_offset(position, c.width * 6, -c.height * 2), TERM_COLOR_WHITE, TERM_BOUNDS_NONE);
    term_print("█", positioned_offset(position, c.width * 2, 0), colored(0, 225, 0));
    term_print("█", positioned_offset(position, c.width * 5, 0), colored(0, 225, 0));
    term_print("██████████████████████", position, transparent(colored(255, 0, 0), 255/4));
    term_print("──────────────────────", position, colored(225, 0, 0));
    
    // case: word-wrap scaled text
    struct term_bounds bounds = bounded(c.width * 8, c.height * 12);
    
    term_fill(positioned(10, 136), bounds.size, colored(255, 255, 225));
    term_set_transform(scaled(2));
    term_printstr("wrap me inside filled area", positioned(10, 136), colored(255, 50, 50), bounds);
    term_set_transform(TERM_TRANSFORM_NONE);
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
