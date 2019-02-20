#include <termlike/termlike.h> // term_*
#include <termlike/animate.h>

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static struct term_animate * ball_position_y;
static float ball_position_y_fixed;

static struct term_animate * fade;

static float ball_velocity = 0;
static float ball_velocity_fixed = 0;

static
void
tick(double const step)
{
    static float const gravity = -9.81f;
    static float const platform_friction = 0.86f;
    
    float const platform_top_y = 200;
    float const speed = 30;

    if (term_key_down(TERM_KEY_UP)) {
        ball_velocity -= speed;
        ball_velocity_fixed -= speed;
    } else if (term_key_down(TERM_KEY_DOWN)) {
        ball_velocity += speed;
        ball_velocity_fixed += speed;
    }
    
    ball_velocity -= gravity;
    ball_velocity_fixed -= gravity;
    
    animate_by(ball_position_y, ball_velocity * step, step);
    
    ball_position_y_fixed += ball_velocity_fixed * step;
    
    float ball_bottom_y;

    animate_get(ball_position_y, &ball_bottom_y);
    
    ball_bottom_y = ball_bottom_y + 8;
    
    if (ball_bottom_y > platform_top_y) {
        ball_velocity *= platform_friction;
        ball_velocity *= -1;
        
        animate_reset(ball_position_y, platform_top_y - 8);
    }
    
    ball_bottom_y = ball_position_y_fixed + 8;
    
    if (ball_bottom_y > platform_top_y) {
        ball_velocity_fixed *= platform_friction;
        ball_velocity_fixed *= -1;
        
        ball_position_y_fixed = platform_top_y - 8;
    }

    float f;

    animate_get(fade, &f);

    // checking prior to animating so that we always get the final frame
    if (f >= TERM_COLOR_OPAQUE) {
        animate_reset(fade, TERM_COLOR_TRANSPARENT);
    } else {
        animate_to(fade, TERM_COLOR_OPAQUE, SECONDS(2), step);
    }
}

static
void
draw(double const interp)
{
    struct term_dimens display;
    
    term_get_display(&display);
    
    int32_t const cx = display.width / 2;
    
    // texts
    term_printstr("This example shows the benefits of interpolated animation.",
                  positioned(12, 12),
                  TERM_COLOR_WHITE,
                  bounded(display.width - 12, display.height - 12));
    
    term_printstr("INTERPOLATED",
                  positioned(cx - 30, 160),
                  TERM_COLOR_WHITE,
                  aligned(TERM_ALIGN_RIGHT));
    
    term_printstr("FIXED (BASIC)",
                  positioned(cx + 30, 160),
                  TERM_COLOR_WHITE,
                  aligned(TERM_ALIGN_LEFT));
    
    // floor
    char const * const floor = "▀▀▀▀▀▀▀▀▀▀▀";
    
    struct term_dimens floor_size;
    
    term_measurestr(floor, TERM_BOUNDS_NONE, &floor_size);
    
    term_printstr(floor,
                  positioned(cx - (floor_size.width / 2), 200),
                  colored(255, 255, 255),
                  TERM_BOUNDS_NONE);
    
    // balls
    char const * const ball = "☻";
    
    struct term_dimens ball_size;
    
    term_measure(ball, &ball_size);
    
    // draw ball using animation values (interpolated)
    int32_t y;
    
    animate_blend(ball_position_y, interp, &y);
    
    term_print(ball,
               positioned(cx - (ball_size.width / 2) - 6, y),
               colored(255, 255, 255));
    
    // draw ball using simply animated values
    y = (int32_t)ball_position_y_fixed;
    
    term_print(ball,
               positioned(cx - (ball_size.width / 2) + 6, y),
               colored(255, 255, 255));


    int32_t a;

    animate_blend(fade, interp, &a);

    term_fill(positioned(20, 60), sized(8, 8), transparent(TERM_COLOR_WHITE, a));
}

int32_t
main(void)
{
    if (!term_open(defaults("Termlike: Bounce"))) {
        exit(EXIT_FAILURE);
    }
    
    term_set_ticking(tick);
    term_set_drawing(draw);
 
    ball_position_y = animated(160);
    ball_position_y_fixed = 160;

    fade = animated(TERM_COLOR_TRANSPARENT);
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        // the benefit of the animation system is that it smoothly interpolates
        // between values, even at weird frequencies (like 15 hz, as seen below)
        // effectively, this means that movement remains smooth even when the
        // framerate is inconsistent; the good old `pos += speed * step;`
        // only works as long as everything runs consistently fast
        term_run(15);
    }
    
    animate_release(ball_position_y);
    animate_release(fade);

    term_close();
    
    return 0;
}
