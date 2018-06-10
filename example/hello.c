#include <termlike/termlike.h> // term_*

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

int32_t
main(void)
{
    term_open(term_settings("Termlike"));
    
    while (!term_should_close()) {
        term_render();
    }
    
    term_close();
    
    return 0;
}
