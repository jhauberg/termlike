#include <termlike/termlike.h> // term_*

#include <stdlib.h> // exit, EXIT_FAILURE
#include <stdint.h> // int32_t
#include <stdbool.h> // bool

int32_t
main(void)
{
    if (!term_open(term_defaults("Termlike"))) {
        exit(EXIT_FAILURE);
    }
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_render();
    }
    
    term_close();
    
    return 0;
}
