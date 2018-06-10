#include <termlike/termlike.h> // term_*

#include <stdint.h> // int32_t
#include <stdbool.h> // bool

int32_t
main(void)
{
    term_open(term_settings("Termlike"));
    
    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_render();
    }
    
    term_close();
    
    return 0;
}
