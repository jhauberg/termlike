#include "cursor.h" // cursor, cursor_*

#include <termlike/bounds.h> // term_bounds

#include <stdint.h> // uint32_t
#include <stdbool.h> // bool

static bool cursor_break_if_needed(struct cursor *);
static void cursor_break(struct cursor *);

void
cursor_start(struct cursor * const cursor,
             struct term_bounds const bounds,
             float const width,
             float const height)
{
    cursor->offset.x = 0;
    cursor->offset.y = 0;
    cursor->offset.line = 0;
    
    cursor->bounds = bounds;
    
    cursor->width = width;
    cursor->height = height;
}

void
cursor_advance(struct cursor * const cursor,
               struct cursor_offset * const offset,
               uint32_t const character)
{
    if (character != '\n') {
        cursor_break_if_needed(cursor);
    }
    
    offset->line = cursor->offset.line;
    offset->x = cursor->offset.x;
    offset->y = cursor->offset.y;
        
    if (character == '\n') {
        cursor_break(cursor);
        
        return;
    }
    
    if (cursor_break_if_needed(cursor)) {
        return;
    }

    cursor->offset.x += cursor->width;
}

bool
cursor_is_out_of_bounds(struct cursor const * const cursor)
{
    if (cursor->bounds.size.height != TERM_BOUNDS_UNBOUNDED) {
        float const bottom = cursor->offset.y + cursor->height;
        
        if (bottom > cursor->bounds.size.height) {
            return true;
        }
    }
    
    return false;
}

static
bool
cursor_break_if_needed(struct cursor * const cursor)
{
    if (cursor->bounds.size.width != TERM_BOUNDS_UNBOUNDED) {
        float const right = cursor->offset.x;
        
        if (right >= cursor->bounds.size.width) {
            cursor_break(cursor);
            
            return true;
        }
    }
    
    return false;
}

static
void
cursor_break(struct cursor * const cursor)
{
    cursor->offset.y += cursor->height;
    cursor->offset.x = 0;
    
    cursor->offset.line += 1;
}
