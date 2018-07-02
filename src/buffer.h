#pragma once

#include <stdint.h> // int32_t, uint32_t, size_t

struct buffer;

/**
 * Represents an offset for a printable character in a buffer.
 */
struct buffer_offset {
    int32_t x;
    int32_t y;
};

/**
 * Represents a function invoked for each printable character in a buffer.
 */
typedef void buffer_callback(uint32_t character, void *);

struct buffer * buffer_init(void);
void buffer_release(struct buffer *);

/**
 * Copy a string to a buffer, preparing it for printing.
 *
 * Before copying, the string is validated against buffer limitations.
 * If valid, the string is copied and the buffer is zero-padded appropriately.
 *
 * If the string exceeds buffer limits, the string is only partially copied.
 */
void buffer_copy(struct buffer *, char const *);
/**
 * Apply word-wrapping to the text contents of a buffer.
 *
 * This function will alter the contents of the buffer by replacing whitespaces
 * with linebreaks in cases where wrapping is required.
 */
void buffer_wrap(struct buffer *, size_t limit);

/**
 * Run through all printable characters in a buffer and issue a
 * callback for each.
 *
 * Functions that require stateful callbacks can provide a generic void pointer
 * that will be passed along with each issued callback.
 */
void buffer_foreach(struct buffer const *, buffer_callback *, void *);
