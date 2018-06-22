#include "../timer.h" // timer, timer_*

#include <stdlib.h> // malloc, free
#include <stdint.h> // uint16_t
#include <stdbool.h> // bool

#include <math.h> // fabs

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wdocumentation"
#endif

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h> // glfwGetTime

#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

struct timer {
    double time;
    double previous;
    double lag;
    double step;
    double scale;
};

static double const maximum_frame_time = 1.0 / 4;

struct timer *
timer_init(void)
{
    struct timer * const timer = malloc(sizeof(struct timer));
    
    timer->scale = 1;
    
    timer_reset(timer);
    
    return timer;
}

void
timer_release(struct timer * const timer)
{
    free(timer);
}

void
timer_reset(struct timer * const timer)
{
    timer->time = 0;
    timer->previous = 0;
    timer->step = 0;
    timer->lag = 0;
    
    glfwSetTime(0);
}

void
timer_begin(struct timer * const timer)
{
    double const time = glfwGetTime();
    double time_elapsed = time - timer->previous;
    
    if (time_elapsed > maximum_frame_time) {
        time_elapsed = maximum_frame_time;
    }
    
    timer->lag += fabs(time_elapsed * timer->scale);
    timer->previous = time;
}

void
timer_end(struct timer * const timer, double * const interpolation)
{
    *interpolation = timer->lag / timer->step;
}

bool
timer_tick(struct timer * const timer,
           uint16_t const frequency,
           double * const step)
{
    timer->step = 1.0 / frequency;
    
    *step = timer->step;
    
    if (timer->lag >= timer->step) {
        timer->lag -= timer->step;
        
        timer->time += timer->step;
        
        return true;
    }
    
    return false;
}

void
timer_set_scale(struct timer * const timer, double const scale)
{
    timer->scale = scale;
}

double
timer_time(struct timer const * const timer)
{
    return timer->time;
}

double
timer_since(struct timer const * const timer, double const time)
{
    return timer->time - time;
}
