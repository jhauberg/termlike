#include "cursor.h" // cursor, cursor_*

#include <termlike/bounds.h> // term_bounds

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

static void cursor_break_if_needed(struct cursor *, struct term_bounds);
static void cursor_break(struct cursor *);

void
cursor_start(struct cursor * const cursor,
             int32_t const width,
             int32_t const height)
{
    cursor->offset.x = 0;
    cursor->offset.y = 0;
    
    cursor->width = width;
    cursor->height = height;
}

void
cursor_advance(struct cursor * const cursor,
               struct term_bounds const bounds,
               uint32_t const character)
{
    if (character == '\n') {
        cursor_break(cursor);
        
        return;
    }
    
    cursor->offset.x += cursor->width;
    
    // this is needed even for word-wrapping, as sometimes
    // a word cannot be wrapped and must be broken up
    cursor_break_if_needed(cursor, bounds);
}

bool
cursor_is_out_of_bounds(struct cursor const * const cursor,
                        struct term_bounds const bounds)
{
    if (bounds.height != TERM_BOUNDS_UNBOUNDED) {
        int32_t const bottom = cursor->offset.y + cursor->height;
        
        if (bottom > bounds.height) {
            return true;
        }
    }
    
    return false;
}

static
void
cursor_break_if_needed(struct cursor * const cursor,
                       struct term_bounds const bounds)
{
    if (bounds.width != TERM_BOUNDS_UNBOUNDED) {
        int32_t const right = cursor->offset.x + cursor->width;
        
        if (right > bounds.width) {
            cursor_break(cursor);
        }
    }
}

static
void
cursor_break(struct cursor * const cursor)
{
    cursor->offset.y += cursor->height;
    cursor->offset.x = 0;
}
