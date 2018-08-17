#pragma once

#include <termlike/layer.h> // term_layer :completeness
#include <termlike/color.h> // term_color :completeness
#include <termlike/bounds.h> // term_bounds :completeness
#include <termlike/transform.h> // term_transform :completeness

#include <stdint.h> // uint64_t
#include <stddef.h> // size_t

struct command_buffer;

struct command_index {
    // note that the order of fields is important
    // going from top to bottom, top is least significant and bottom is most
    // this effectively means Z-value is more important than call order
    // but two indices with identical Z-value will fall back to call order
    uint32_t order;
    float z;
};

struct command {
    struct term_color const color;
    struct term_bounds const bounds;
    struct term_transform const transform;
    struct term_layer const layer;
    char const * text;
    uint64_t const index;
    float x, y;
};

typedef void command_callback(struct command const *);

struct command_buffer * command_init(void);
void command_release(struct command_buffer *);

void command_push(struct command_buffer *, struct command);
void command_flush(struct command_buffer *, command_callback *);

void command_memuse(struct command_buffer const *, size_t * memory);

inline
struct command_index *
command_index(struct command const * const command)
{
    return (struct command_index *)&command->index;
}

