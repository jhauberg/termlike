#pragma once

#include <termlike/position.h> // term_position :completeness
#include <termlike/color.h> // term_color :completeness
#include <termlike/bounds.h> // term_bounds :completeness
#include <termlike/transform.h> // term_transform :completeness

#include <stdint.h> // uint64_t, size_t

struct command_buffer;

struct command_index {
    uint32_t order;
    float z;
};

struct command {
    struct term_position const position;
    struct term_color const color;
    struct term_bounds const bounds;
    struct term_transform const transform;
    char const * text;
    uint64_t const index;
};

typedef void command_callback(struct command const *);

struct command_buffer * command_init(void);
void command_release(struct command_buffer *);

void command_push(struct command_buffer *, struct command);
void command_flush(struct command_buffer *, command_callback *);

void command_memuse(struct command_buffer const *, size_t *);

struct command_index * command_index(struct command const *);
