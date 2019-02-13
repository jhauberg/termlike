#include "command.h" // command, command_index, command_*

#include <termlike/layer.h> // term_layer

#include <stdlib.h> // malloc, free, qsort
#include <stdint.h> // uint16_t, uint32_t, UINT8_MAX, UINT16_MAX
#include <stddef.h> // size_t, NULL

#ifdef DEBUG
 #include <assert.h> // assert
#endif

#ifdef _WIN32
 #include <string.h> // memcpy
#endif

#define MAX_CAPACITY UINT16_MAX

struct command_index {
    // note that the order of fields is important
    // going from top to bottom, top is least significant and bottom is most
    // this effectively means Z-value is more important than call order
    // but two indices with identical Z-value will fall back to call order
    uint16_t order;
    uint16_t depth;
};

struct command_buffer {
    struct command * commands;
    size_t capacity;
    size_t count;
};

static inline struct command_index command_int_to_index(uint32_t index);
static inline uint32_t command_index_to_int(struct command_index);
static int32_t command_compare(void const *, void const *);

struct command_buffer *
command_init(void)
{
#ifdef DEBUG
    assert(sizeof(struct command_index) == sizeof(uint32_t));
#endif
    
    struct command_buffer * const buf = malloc(sizeof(struct command_buffer));
    
    buf->count = 0;
    buf->capacity = UINT8_MAX + 1;

    buf->commands = malloc(sizeof(struct command) * buf->capacity);
    
    return buf;
}

void
command_release(struct command_buffer * const buffer)
{
    free(buffer->commands);
    free(buffer);
}

#ifdef TERM_INCLUDE_PROFILER
void
command_get_capacity(struct command_buffer const * const buffer,
                     size_t * const used,
                     size_t * const cap)
{
    *used = sizeof(struct command) * buffer->count;
    *cap = sizeof(struct command) * MAX_CAPACITY;
}
#endif

void
command_push(struct command_buffer * const buffer,
             struct command const command)
{
    if (buffer->capacity == buffer->count) {
        size_t expanded_capacity = buffer->capacity * 2;

        if (expanded_capacity > MAX_CAPACITY) {
            expanded_capacity = MAX_CAPACITY;
        }

#ifdef DEBUG
        assert(expanded_capacity > buffer->count);
#endif

        buffer->capacity = expanded_capacity;
        buffer->commands = realloc(buffer->commands,
                                   sizeof(struct command) * buffer->capacity);
    }
    
#ifdef _WIN32
    memcpy(&buffer->commands[buffer->count], &command, sizeof(struct command));
#else
    buffer->commands[buffer->count] = command;
#endif
    
    buffer->count += 1;
}

void
command_flush(struct command_buffer * const buffer,
              command_callback * const callback)
{
    if (callback != NULL) {
        qsort(buffer->commands,
              buffer->count,
              sizeof(struct command),
              command_compare);

        for (size_t i = 0; i < buffer->count; i++) {
            struct command * const command = &buffer->commands[i];
            
            callback(command);
        }
    }
    
    buffer->count = 0;
}

uint32_t
command_next_layered_index(struct command_buffer const * const buffer,
                           struct term_layer const layer)
{
    return command_index_to_int((struct command_index) {
        .order = (uint16_t)buffer->count,
        .depth = (layer.index * UINT8_MAX) + layer.depth
    });
}

float
command_index_to_z(uint32_t const index)
{
    struct command_index const cmd_index = command_int_to_index(index);

    return (float)cmd_index.depth / UINT16_MAX;
}

static inline
struct command_index
command_int_to_index(uint32_t const index)
{
    struct command_index * const index_ptr = (struct command_index *)&index;

    return *index_ptr;
}

static inline
uint32_t
command_index_to_int(struct command_index const index)
{
    uint32_t * const index_ptr = (uint32_t *)&index;

    return *index_ptr;
}

static
int32_t
command_compare(void const * const cmd,
                void const * const other_cmd)
{
    struct command const * lhs = (struct command *)cmd;
    struct command const * rhs = (struct command *)other_cmd;
    
    if (lhs->index < rhs->index) {
        return -1;
    } else if (lhs->index > rhs->index) {
        return 1;
    }
    
    return 0;
}
