#include <termlike/termlike.h> // term_*
#include <termlike/animate.h>

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool :completeness

static struct term_animate * ball_position_y;
static float ball_position_y_fixed;

static float ball_velocity = 0;
static float ball_velocity_fixed = 0;

static
void
tick(double const step)
{
    static float const gravity = -9.81f;
    static float const platform_friction = 0.86f;
    
    float const platform_top_y = 200;
    
    if (term_key_down(TERM_KEY_UP)) {
        ball_velocity -= 25;
        ball_velocity_fixed -= 25;
    } else if (term_key_down(TERM_KEY_DOWN)) {
        ball_velocity += 25;
        ball_velocity_fixed += 25;
    }
    
    ball_velocity -= gravity;
    ball_velocity_fixed -= gravity;
    
    animate_by(ball_position_y, ball_velocity, step);
    
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
}

static
void
draw(double const interp)
{
    int32_t dw, dh;
    
    term_get_display(&dw, &dh);
 
    int32_t const cx = dw / 2;
    
    // texts
    term_printstr(positioned(12, 12),
                  TERM_COLOR_WHITE,
                  bounded(dw - 12, dh - 12),
                  "This example shows the benefits of interpolated animation.");
 
    term_printstr(positioned(cx - 30, 160),
                  TERM_COLOR_WHITE,
                  aligned(TERM_ALIGN_RIGHT),
                  "INTERPOLATED");
    
    term_printstr(positioned(cx + 30, 160),
                  TERM_COLOR_WHITE,
                  aligned(TERM_ALIGN_LEFT),
                  "FIXED (BASIC)");
    
    // floor
    char const * const floor = "▀▀▀▀▀▀▀▀▀▀▀";
    
    int32_t w, h;
    
    term_measure(floor, &w, &h);
    
    term_print(positioned(cx - (w / 2), 200),
               colored(255, 255, 255),
               floor);
    
    // balls
    char const * const ball = "☻";
    
    term_measure(ball, &w, &h);
    
    // draw ball using animation values (interpolated)
    int32_t y;
    
    animate_blend(ball_position_y, interp, &y);
    
    term_print(positioned(cx - (w / 2) - 6, y),
               colored(255, 255, 255),
               ball);
    
    // draw ball using simply animated values
    y = (int32_t)ball_position_y_fixed;
    
    term_print(positioned(cx - (w / 2) + 6, y),
               colored(255, 255, 255),
               ball);
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
    
    term_close();
    
    return 0;
}
