#include "command.h" // command, command_index, command_*

#include "internal.h" // layer_z

#include <termlike/layer.h> // term_layer

#include <stdlib.h> // malloc, free, qsort
#include <stdint.h> // uint32_t
#include <stddef.h> // size_t, NULL

#ifdef DEBUG
 #include <assert.h> // assert
#endif

#ifdef _WIN32
 #include <string.h> // memcpy
#endif

struct command_index {
    // note that the order of fields is important
    // going from top to bottom, top is least significant and bottom is most
    // this effectively means Z-value is more important than call order
    // but two indices with identical Z-value will fall back to call order
    uint32_t order;
    float z;
};

struct command_buffer {
    struct command * commands;
    uint32_t count;
    uint32_t capacity;
};

static int32_t command_compare(void const *, void const *);

struct command_buffer *
command_init(void)
{
#ifdef DEBUG
    assert(sizeof(struct command_index) == sizeof(uint64_t));
#endif
    
    struct command_buffer * const buf = malloc(sizeof(struct command_buffer));
    
    buf->count = 0;
    buf->capacity = 256;

    buf->commands = malloc(sizeof(struct command) * buf->capacity);
    
    return buf;
}

void
command_release(struct command_buffer * const buffer)
{
    free(buffer->commands);
    free(buffer);
}

void
command_push(struct command_buffer * const buffer,
             struct command const command)
{
#ifdef DEBUG
    assert(buffer->count <= buffer->capacity);
#endif
    
    if (buffer->capacity == buffer->count) {
        buffer->capacity = buffer->capacity * 2;

        if (buffer->capacity >= UINT16_MAX) {
            buffer->capacity = UINT16_MAX;
        }
        
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
command_get_zindex(uint64_t const index, float * const z)
{
    struct command_index * const idx = (struct command_index *)&index;
    
    *z = idx->z;
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

        for (uint32_t i = 0; i < buffer->count; i++) {
            struct command * const command = &buffer->commands[i];
            
            callback(command);
        }
    }
    
    buffer->count = 0;
}

uint64_t
command_next_index_at(struct command_buffer const * const buffer,
                      struct term_layer const layer)
{
    struct command_index const index = (struct command_index) {
        .order = buffer->count,
        .z = layer_z(layer)
    };
    
    return *((uint64_t *)&index);
}

#ifdef TERM_INCLUDE_PROFILER
void
command_memuse(struct command_buffer const * const buffer, size_t * const size)
{
    *size = sizeof(struct command) * buffer->capacity;
}
#endif

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
