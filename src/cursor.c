#include "cursor.h" // cursor, cursor_*

#include <termlike/bounds.h> // term_bounds

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
               uint32_t const character,
               struct term_bounds const bounds)
{
    (void)bounds;
    
    if (character == '\n') {
        cursor->offset.y += cursor->height;
        cursor->offset.x = 0;
        
        return;
    }
    
    cursor->offset.x += cursor->width;
}
