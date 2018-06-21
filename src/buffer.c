#include "buffer.h"

#include <stdlib.h> // malloc, free
#include <stdint.h> // uint32_t, int32_t

#include <string.h> // strncpy, strlen, memset

#include <utf8.h> // utf8_decode

/**
 * The number of bytes that the internal text buffer must be zero-padded with.
 
 * This is required for UTF8 decoding.
 */
#define BUFFER_PADDING 4
/**
 * The size of the internal text buffer.
 *
 * This value must be at least 1 higher than the amount of required padding.
 */
#define BUFFER_SIZE_MAX 256
/**
 * The maximum length of a printed string.
 */
#define MAX_TEXT_LENGTH (BUFFER_SIZE_MAX - BUFFER_PADDING)

struct buffer {
    char text[BUFFER_SIZE_MAX];
    struct buffer_dimens dimensions;
};

struct buffer *
buffer_init(struct buffer_dimens dimensions)
{
    struct buffer * const buffer = malloc(sizeof(struct buffer));
    
    buffer->dimensions = dimensions;
    
    memset(buffer->text, '\0', sizeof(buffer->text));
    
    return buffer;
}

void
buffer_release(struct buffer * const buffer)
{
    free(buffer);
}

void
buffer_copy(struct buffer * const buffer, char const * const text)
{
    size_t length = strlen(text);
    
    if (length >= MAX_TEXT_LENGTH) {
        length = MAX_TEXT_LENGTH;
    }
    
    strncpy(buffer->text, text, length);
    
    for (uint16_t i = 0; i < BUFFER_PADDING; i++) {
        // null-terminate the buffer and add padding (utf8 decoding)
        buffer->text[length + i] = '\0';
    }
}

void
buffer_characters(struct buffer const * const buffer,
                  buffer_char_callback * const callback,
                  void * const state)
{
    char const * text_ptr = buffer->text;
    
    int32_t decoding_error;
    uint32_t character;
    
    int32_t x = 0;
    
    struct buffer_offset offset = {
        .x = 0,
        .y = 0
    };
    
    while (*text_ptr) {
        // decode and advance the text pointer
        text_ptr = utf8_decode(text_ptr, &character, &decoding_error);
        
        if (decoding_error != 0) {
            // error
            
            break;
        }
        
        if (character == '\n') {
            offset.y += buffer->dimensions.height;
            offset.x = x;
            
            continue;
        } else {
            callback(offset, buffer->dimensions, character, state);
        }
        
        offset.x += buffer->dimensions.width;
    }
}
