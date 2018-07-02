#include "command.h"

#include "internal.h" // layer_z

#include <stdlib.h> // malloc, free, qsort
#include <stdint.h> // uint32_t

struct command_buffer {
    struct command * commands;
    uint32_t count;
    uint32_t capacity;
};

static int32_t command_compare(void const *, void const *);

struct command_buffer *
command_init(void)
{
    struct command_buffer * const buffer = malloc(sizeof(struct command_buffer));
    
    buffer->count = 0;
    buffer->capacity = 64;
    buffer->commands = malloc(sizeof(struct command) * buffer->capacity);
    
    return buffer;
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
    if (buffer->capacity == buffer->count) {
        buffer->capacity = buffer->capacity * 2;

        buffer->commands = realloc(buffer->commands,
                                   sizeof(struct command) * buffer->capacity);
    }
    
    struct command_index * const index = (struct command_index *)&command.index;
    
    index->order = buffer->count;
    index->z = layer_z(command.position.layer);
    
    buffer->commands[buffer->count] = command;
    buffer->count += 1;
}

void
command_flush(struct command_buffer * const buffer,
              command_callback * const callback)
{
    qsort(buffer->commands,
          buffer->count,
          sizeof(struct command),
          command_compare);
    
    if (callback != NULL) {
        for (uint32_t i = 0; i < buffer->count; i++) {
            callback(&buffer->commands[i]);
        }
    }
    
    buffer->count = 0;
}

struct command_index *
command_index(struct command const * const command)
{
    return (struct command_index *)&command->index;
}

void
command_memuse(struct command_buffer const * const buffer, size_t * const size)
{
    *size = sizeof(struct command) * buffer->capacity;
}

static
int32_t
command_compare(void const * const cmd, void const * const other_cmd)
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
