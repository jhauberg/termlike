#include "buffer.h" // buffer, buffer_*

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
};

struct buffer *
buffer_init(void)
{
    struct buffer * const buffer = malloc(sizeof(struct buffer));
    
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
buffer_wrap(struct buffer * const buffer, size_t const limit)
{
    size_t num_characters = 0;
    
    int32_t decoding_error;
    uint32_t character;
    
    char * next = buffer->text;
    
    while (*next) {
        char * previous = next;
        
        next = utf8_decode(next, &character, &decoding_error);
        
        if (decoding_error != 0) {
            break;
        }
        
        // we successfully read one character from the buffer
        num_characters += 1;
        
        if (character == '\n') {
            // we hit an explicit linebreak,
            // so we can assume that a new line starts here
            num_characters = 0;
        }

        if (num_characters < limit) {
            // we've not yet reached the limit for number of characters per line
            // so we just continue with reading the next character
            continue;
        }
        
        // we've reached the limit for number of characters per line,
        // so backtrack to find a previous whitespace where we can break
        // note that in a case where no available whitespace can be found,
        // no breaks will be inserted
        while (*previous) {
            if (*previous == ' ') {
                // and replace it with a line break
                *previous = '\n';
                
                // make sure to begin the next line from this point
                // we can be sure that this is not "mid-character" for
                // larger-sized characters, because we specifically found
                // the whitespace
                next = previous;
                
                break;
            }
            
            // note that we're decrementing by a single byte at a time,
            // and *not* by 1 character (because each character may be up to
            // 4 bytes long); in this case, it doesn't matter, because we're
            // looking for a specific single-byte character: the whitespace
            previous--;
        }
    }
}

void
buffer_foreach(struct buffer const * const buffer,
               buffer_char_callback * const callback,
               void * const state)
{
    int32_t decoding_error;
    uint32_t character;
    
    // remove constness to iterate through the buffer
    char * next = ((struct buffer *)buffer)->text;
    
    while (*next) {
        // decode and advance the text pointer
        next = utf8_decode(next, &character, &decoding_error);
        
        if (decoding_error != 0) {
            // error
            
            break;
        }
        
        callback(character, state);
    }
}
