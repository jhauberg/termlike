#include "buffer.h" // buffer, buffer_*

#include <stdlib.h> // malloc, free
#include <stdint.h> // uint32_t, int32_t
#include <stddef.h> // size_t

#include <string.h> // strlen, memset, memcpy

#include <utf8.h> // utf8_decode

#ifdef DEBUG
 #include <assert.h> // assert
#endif

/**
 * The maximum number of characters in a printed string.
 */
#define MAX_TEXT_LENGTH (4096)
/**
 * The size (in bytes) of a single UTF8 encoded character.
 */
#define BUFFER_CHAR_SIZE (sizeof(uint32_t))
/**
 * The size of the internal text buffer.
 *
 * This value must be at least 1 higher than the required amount of padding (4).
 */
#define BUFFER_SIZE (MAX_TEXT_LENGTH * BUFFER_CHAR_SIZE)

struct buffer {
    uint32_t decoded[BUFFER_SIZE];
    char const * text;
};

static void buffer_decode(struct buffer *, char * text);

struct buffer *
buffer_init(void)
{
    struct buffer * const buffer = malloc(sizeof(struct buffer));
    
    memset(buffer->decoded, 0, sizeof(buffer->decoded));
    
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
#ifdef DEBUG
    assert(text != NULL);
#endif
    size_t const length = strlen(text);
#ifdef DEBUG
    assert(length <= MAX_TEXT_LENGTH);
#endif
    buffer->text = text;
    
    char content[BUFFER_SIZE];

    // copy over entire string as-is
    memcpy(&content, text, length);
    // pad the buffer so that there's enough room to decode each character
    memset(&content[length], '\0', BUFFER_CHAR_SIZE);
    
    // clear any previously decoded content
    // (but only clearing as much as we need to, e.g. not the entire buffer,
    //  so "garbage" content may remain from previous copies- this shouldn't
    //  matter as long as we pad the real content appropriately)
    // note that we approximate the length to clear by treating
    // each byte as 1 character; that isn't necesarilly correct,
    // but in any case it will be better than clearing too little
    size_t const n = (length * BUFFER_CHAR_SIZE) + BUFFER_CHAR_SIZE;
    
    memset(buffer->decoded, 0, n);
    
    buffer_decode(buffer, content);
}

void
buffer_wrap(struct buffer * const buffer, size_t const limit)
{
    size_t num_characters = 0;
    
    uint32_t character = 0;
    uint32_t * next = buffer->decoded;
    
    while (*next) {
        uint32_t * previous = next;
        
        character = *next++;
        
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
               buffer_callback * const callback,
               void * const state)
{
    uint16_t i = 0;
    uint32_t glyph = buffer->decoded[i];
    
    while (glyph != 0) {
        callback(glyph, state);
        
        i++;
        
        glyph = buffer->decoded[i];
    }
}

static
void
buffer_decode(struct buffer * const buffer, char * const text)
{
    int32_t error = 0;
    uint16_t i = 0;
    
    char * next = text;
    
    while (*next) {
        // decode and advance the text pointer
        next = utf8_decode(next, &buffer->decoded[i], &error);
#ifdef DEBUG
        assert(error == 0);
#endif
        
        i++;
    }
}
