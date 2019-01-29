#pragma once

#include <termlike/position.h> // term_location :completeness
#include <termlike/color.h> // term_color :completeness
#include <termlike/bounds.h> // term_bounds :completeness
#include <termlike/transform.h> // term_transform :completeness

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t

struct term_layer;

struct command_buffer;

struct command {
    char const * text;
    struct term_transform transform;
    struct term_location origin;
    struct term_bounds bounds;
    struct term_color color;
    uint32_t index;
};

typedef void command_callback(struct command const *);

struct command_buffer * command_init(void);
void command_release(struct command_buffer *);

void command_push(struct command_buffer *, struct command);
void command_flush(struct command_buffer *, command_callback *);

float command_index_to_z(uint32_t index);

uint32_t command_next_layered_index(struct command_buffer const *,
                                    struct term_layer);

#ifdef TERM_INCLUDE_PROFILER
void command_memuse(struct command_buffer const *, size_t * memory);
#endif
