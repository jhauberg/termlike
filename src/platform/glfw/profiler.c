#include "../profiler.h" // profiler_*

#include <stdint.h> // uint16_t, UINT16_MAX

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

static double const frame_average_interval = 1.0; // in seconds

static double last_frame_time = 0;
static double last_average_time = 0;

static uint16_t frame_samples = 0; // sample frames to calculate average
static uint16_t frame_sample_count = 0;

static double frame_time = 0;
static double frame_time_avg = 0;

static double frame_time_samples = 0; // sample frames to calculate average
static uint16_t frame_time_sample_count = 0;

static struct profiler_stats stats;

void
profiler_reset(void)
{
    stats.frame_time = 0;
    stats.frame_time_avg = 0;
    stats.frames_per_second = 0;
    stats.frames_per_second_min = UINT16_MAX;
    stats.frames_per_second_max = 0;
    stats.frames_per_second_avg = 0;
}

void
profiler_begin(void)
{
    stats.draw_count = 0;
}

void
profiler_end(void)
{
    double const time = glfwGetTime();
    double const time_since_last_frame = time - last_frame_time;
    
    frame_time = time_since_last_frame;
    last_frame_time = time;
    
    stats.frames_per_second = (uint16_t)(1.0 / frame_time);
    
    if (stats.frames_per_second < stats.frames_per_second_min) {
        stats.frames_per_second_min = stats.frames_per_second;
    }
    
    if (stats.frames_per_second > stats.frames_per_second_max) {
        stats.frames_per_second_max = stats.frames_per_second;
    }
    
    frame_samples += stats.frames_per_second;
    frame_sample_count++;
    
    frame_time_samples += frame_time;
    frame_time_sample_count++;
    
    double const time_since_last_average = time - last_average_time;
    
    if (time_since_last_average >= frame_average_interval) {
        last_average_time = time;
        
        profiler_update_averages();
    }
    
    stats.frame_time = frame_time * 1000;
    stats.frame_time_avg = frame_time_avg * 1000;
}

struct profiler_stats
profiler_stats(void)
{
    return stats;
}

static
void
profiler_update_averages(void)
{
    stats.frames_per_second_avg = frame_samples / frame_sample_count;
    frame_sample_count = 0;
    frame_samples = 0;
    
    frame_time_avg = frame_time_samples / frame_time_sample_count;
    frame_time_sample_count = 0;
    frame_time_samples = 0;
    
    // reset min/max to show rolling stats rather than historically accurate
    // stats (it's more interesting knowing min/max for the current scene
    // than knowing the 9999+ max fps during the first blank frame)
    stats.frames_per_second_min = UINT16_MAX;
    stats.frames_per_second_max = 0;
}
