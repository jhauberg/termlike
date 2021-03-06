#include <termlike/termlike.h> // term_*, TERM_*
#include <termlike/platform/profiler.h> // profiler_*

#include <stdint.h> // uint16_t, int32_t, UINT16_MAX
#include <stdio.h> // sprintf

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wdocumentation"
#endif

#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h> // glfwGetTime

#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

static void profiler_update_averages(void);

static double const frame_average_interval = 0.666; // in seconds

static double last_frame_time = 0;
static double last_average_time = 0;

static uint16_t frames_per_second = 0;

static double frame_time = 0;

static char stats_string[32];

static struct profiler_stats current;

void
profiler_reset(void)
{
    current.draw_count = 0;
    current.frame_time = 0;
    current.frames_per_second = 0;
    current.frames_per_second_min = UINT16_MAX;
    current.frames_per_second_max = 0;
}

void
profiler_begin(void)
{
    current.draw_count = 0;
}

void
profiler_increment_draw_count(uint8_t const amount)
{
    current.draw_count += amount;
}

void
profiler_end(void)
{
    double const time = glfwGetTime();
    double const time_since_last_frame = time - last_frame_time;

    frame_time = time_since_last_frame;
    last_frame_time = time;

    frames_per_second = (uint16_t)(1.0 / frame_time);

    if (frames_per_second < current.frames_per_second_min) {
        current.frames_per_second_min = frames_per_second;
    }

    if (frames_per_second > current.frames_per_second_max) {
        current.frames_per_second_max = frames_per_second;
    }

    double const time_since_last_average = time - last_average_time;

    if (time_since_last_average >= frame_average_interval) {
        last_average_time = time;

        profiler_update_averages();
    }

    current.frame_time = frame_time * 1000;
}

void
profiler_draw(void)
{
    struct term_dimens display;

    term_get_display(&display);

    struct term_transform transform;

    term_get_transform(&transform);
    term_set_transform(TERM_TRANSFORM_NONE);

    struct term_dimens c;

    term_measure(TERM_SINGLE_GLYPH, &c);

    int32_t const y = display.height - c.height;
    int32_t const pad = 1;

    term_fill(positionedz(0, y - pad, layered_below(TERM_LAYER_TOP)),
              sized(display.width, c.height + pad),
              colored(255, 255, 225));

    struct term_color const text_color = colored(55, 55, 55);
    struct term_layer const text_layer = TERM_LAYER_TOP;

    term_printstr(stats_string,
                  positionedz(display.width - pad, y, text_layer),
                  text_color,
                  aligned(TERM_ALIGN_RIGHT));

    term_printstr("` to disable",
                  positionedz(0, y, text_layer),
                  text_color,
                  aligned(TERM_ALIGN_LEFT));

    term_set_transform(transform);
}

void
profiler_sum(struct profiler_stats const stats, float const load)
{
    uint16_t load_pct = (uint16_t)(load * 100);

    if (load_pct < 1) {
        load_pct = 1;
    }

    sprintf(stats_string,
            "%dFPS %dxDRAW %d%%LOAD",
            stats.frames_per_second,
            stats.draw_count,
            load_pct);
}

struct profiler_stats
profiler_stats(void)
{
    return current;
}

static
void
profiler_update_averages(void)
{
    current.frames_per_second = frames_per_second;

    // reset min/max to show rolling stats rather than historically accurate
    // stats (it's more interesting knowing min/max for the current scene
    // than knowing the 9999+ max fps during the first blank frame)
    current.frames_per_second_min = UINT16_MAX;
    current.frames_per_second_max = 0;
}
