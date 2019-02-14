#pragma once

#include <stdint.h> // uint32_t
#include <stddef.h> // size_t

/**
 * The maximum number of characters a buffer will hold.
 */
#define MAX_TEXT_LENGTH (4096)

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
 * If length is not zero, only characters up to that point is copied. Otherwise
 * entire string is copied (null-terminated).
 */
void buffer_copy(struct buffer *, char const *, size_t length);
/**
 * Apply word-wrapping to the text contents of a buffer.
 *
 * This function alters the contents of the buffer by replacing whitespaces
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
