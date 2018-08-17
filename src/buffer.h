#pragma once

#include <stdint.h> // uint32_t
#include <stddef.h> // size_t

/**
 * The size of the internal text buffer.
 *
 * This value must be at least 1 higher than the required amount of padding (4).
 */
#define BUFFER_SIZE_MAX 4096

struct buffer;

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
