#pragma once

#include <stdint.h> // uint8_t, uint16_t

struct profiler_stats {
    double frame_time;
    double frame_time_avg;
    uint16_t frames_per_second;
    uint16_t frames_per_second_min;
    uint16_t frames_per_second_max;
    uint16_t frames_per_second_avg;
};

void profiler_reset(void);

void profiler_begin(void);
void profiler_end(void);

struct profiler_stats profiler_stats(void);
