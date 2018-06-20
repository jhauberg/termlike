#pragma once

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

struct buffer;

/**
 * Represents an offset for a printable character in a buffer.
 */
struct buffer_offset {
    int32_t x;
    int32_t y;
};

/**
 * Represents the dimensions (in pixels) of each character in a buffer.
 */
struct buffer_dimens {
    int32_t width;
    int32_t height;
};

/**
 * Represents a function invoked for each printable character in a buffer.
 */
typedef void buffer_char_callback(struct buffer_offset,
                                  struct buffer_dimens,
                                  uint32_t c,
                                  void *);

struct buffer * buffer_init(struct buffer_dimens);
void buffer_release(struct buffer *);

/**
 * Copy a string to a buffer, preparing it for printing.
 *
 * Before copying, the string is validated against buffer limitations.
 * If valid, the string is copied and the buffer is zero-padded appropriately.
 *
 * Return true if string was copied, false otherwise.
 */
bool buffer_copy(struct buffer *, char const *);

/**
 * Run through all printable characters in a buffer and issue a
 * callback at each character offset.
 *
 * The buffer is read from left to right, and newlines do not trigger callbacks.
 *
 * Functions that require stateful unrolls can provide a generic void pointer
 * that will be passed along with each callback.
 */
void buffer_characters(struct buffer *, buffer_char_callback *, void *);
