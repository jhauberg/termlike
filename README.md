# Termlike

[![code style: compliant](https://img.shields.io/badge/code%20style-compliant-000000.svg)](https://github.com/jhauberg/comply)

A low-profile library for building high-performance and cross-platform games in the aesthetic of traditional text-based games, but with the benefits of modern graphics.

The name *Termlike* is in reference to the [*Roguelike*](https://en.wikipedia.org/wiki/Roguelike) genre of games, which historically has roots in text-based terminals.

Termlike is written entirely in **C99** and requires **OpenGL 3.3 (Core Profile)**.

## Usage

Termlike is intended as a static library (`libterm.a`) that you link into your program.

Here's the smallest program that just runs a terminal window until user presses <kbd>Esc</kbd>:

```c
#include <termlike/termlike.h>

int
main(void)
{
    term_open(term_defaults("Termlike"));

    while (!term_is_closing()) {
        if (term_key_down(TERM_KEY_ESCAPE)) {
            term_set_closing(true);
        }
        
        term_run(TERM_FREQUENCY_DEFAULT);
    }
    
    term_close();
    
    return 0;
}
```

Take a look in [example](/example) for more usage samples.

## Building

This project is in a very early stage of development. For the time being, build instructions are not included.

### Dependencies

A few dependencies are required to keep the scope of the project down. Most of these are small and already included as part of the repository (see [external](/external)).

**Requires linkage:**

* [`glfw/GLFW3`](https://github.com/glfw/glfw) handles **cross-platform window creation**

**Included:**

* [`skaslev/gl3w`](https://github.com/skaslev/gl3w) for **OpenGL Core Profile** header loading
* [`datenwolf/linmath`](https://github.com/datenwolf/linmath.h) provides **math functions**
* [`nothings/stb_image`](https://github.com/nothings/stb) provides **image loading** capabilities (png)
* [`skeeto/branchless-utf8`](https://github.com/skeeto/branchless-utf8) provides **UTF8 decoding**

## License

Termlike is a Free Open-Source Software project released under the [MIT License](LICENSE).
