#pragma once

#include <stdint.h> // uint8_t, uint16_t

struct profiler_stats {
    double frame_time;
    uint16_t frames_per_second;
    uint16_t frames_per_second_min;
    uint16_t frames_per_second_max;
    uint16_t draw_count;
};

void profiler_reset(void);

void profiler_begin(void);
void profiler_end(void);

void profiler_sum(struct profiler_stats, float load);

void profiler_draw(void);
void profiler_increment_draw_count(uint8_t amount);

struct profiler_stats profiler_stats(void);
