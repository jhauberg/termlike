#include "cursor.h" // cursor, cursor_*

#include <termlike/bounds.h> // term_bounds

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

static void cursor_break_if_needed(struct cursor *, struct term_bounds);
static void cursor_break(struct cursor *);

void
cursor_start(struct cursor * const cursor,
             float const width,
             float const height)
{
    cursor->x = 0;
    cursor->y = 0;
    
    cursor->width = width;
    cursor->height = height;
    
    cursor->breaks = 0;
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
    
    cursor->x += cursor->width;
    
    // this is needed even for word-wrapping, as sometimes
    // a word cannot be wrapped and must be broken up
    cursor_break_if_needed(cursor, bounds);
}

bool
cursor_is_out_of_bounds(struct cursor const * const cursor,
                        struct term_bounds const bounds)
{
    if (bounds.size.height != TERM_BOUNDS_UNBOUNDED) {
        float const bottom = cursor->y + cursor->height;
        
        if (bottom > bounds.size.height) {
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
    if (bounds.size.width != TERM_BOUNDS_UNBOUNDED) {
        float const right = cursor->x + cursor->width;
        
        if (right > bounds.size.width) {
            cursor_break(cursor);
        }
    }
}

static
void
cursor_break(struct cursor * const cursor)
{
    cursor->y += cursor->height;
    cursor->x = 0;
    
    cursor->breaks += 1;
}
