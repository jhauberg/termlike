#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static
void
draw(double const interp)
{
    (void)interp;

    char const * const background = "█";

    struct term_dimens window;

    term_get_display(&window);

    struct term_dimens c;

    term_measure(background, &c);

    int32_t const columns = window.width / c.width;
    int32_t const rows = window.height / c.height;

    struct term_layer const background_layer = layered(0);
    struct term_layer const foreground_layer = layered_above(background_layer);

    for (uint16_t column = 0; column < columns; column++) {
        for (uint16_t row = 0; row < rows; row++) {
            int32_t x = column * c.width;
            int32_t y = row * c.height;

            struct term_color const background_color = colored(rand() % 255,
                                                               rand() % 255,
                                                               rand() % 255);

            term_print(background,
                       positionedz(x, y, background_layer),
                       background_color);
        }
    }

    for (uint16_t column = 0; column < columns; column++) {
        for (uint16_t row = 0; row < rows; row++) {
            int32_t x = column * c.width;
            int32_t y = row * c.height;

            struct term_color foreground_color = colored(rand() % 255,
                                                         rand() % 255,
                                                         rand() % 255);

            term_print("○",
                       positionedz(x, y, foreground_layer),
                       foreground_color);
        }
    }

    for (uint16_t column = 0; column < columns; column++) {
        for (uint16_t row = 0; row < rows; row++) {
            int32_t x = column * c.width;
            int32_t y = row * c.height;

            struct term_color foreground_color = colored(rand() % 255,
                                                         rand() % 255,
                                                         rand() % 255);

            term_set_transform(rotated(rand() % 360, TERM_ROTATE_CHARACTERS));

            term_print("•",
                       positionedz(x, y, layered_above(foreground_layer)),
                       transparent(foreground_color, rand() % 255));
        }
    }

    // note the 3 identical loops:
    // we could actually achieve same output with only 1 loop, however,
    // since each print occur on a different layer, this way prevents
    // interleaved print commands, resulting in better FPS, because there is
    // less sorting to do per frame

    term_set_transform(TERM_TRANSFORM_NONE);
}

int32_t
main(void)
{
    struct term_settings settings = defaults("Termlike: Performance test");

    settings.vsync = false;
    settings.pixel_size = 1;

    if (!term_open(settings)) {
        exit(EXIT_FAILURE);
    }

    term_set_drawing(draw);

    srand(1234);

    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }

        term_run(TERM_FREQUENCY_NONE);
    }

    term_close();

    return 0;
}
