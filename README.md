# Termlike

[![code style: compliant](https://img.shields.io/badge/code%20style-compliant-000000.svg)](https://github.com/jhauberg/comply)

A low-profile library for building high-performance and cross-platform games in the aesthetic of traditional text-based applications, but with the benefits of modern graphics technology.

The name *Termlike* is in reference to the [*Roguelike*](https://en.wikipedia.org/wiki/Roguelike) genre of games, which historically has roots in text-based terminals.

Termlike is written in **C99** and requires **OpenGL 3.3** or later.

<img src="assets/screenshot.png" width="432" height="374">

### Limitations

Termlike specializes in only one thing; getting character glyphs on the screen. As such, it has limitations.

**256 Glyphs**

Termlike *only* supports the 256 glyphs defined by [Codepage 437](https://en.wikipedia.org/wiki/Code_page_437), and provides a built-in font that resembles the one found on the original [IBM PC](https://en.wikipedia.org/wiki/IBM_PC).

There is no support for custom fonts or tiles, nor any plans for it.

**Display sizes**

There are only a few available display sizes (i.e. window dimensions). They were chosen as the best fits for the embedded font and are not customizable; similarly, there is no support for resizable windows.

## Usage

Termlike is intended as a static library (`libterm.a`) that you link into your program.

Here's the smallest program that just runs a terminal window until user presses <kbd>Esc</kbd>:

```c
#include <termlike/termlike.h>

int
main(void)
{
    term_open(defaults("Termlike"));

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

Take a look in [examples](/example) for more usage samples.

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

## Technicalities

**Sprite batching**

Behind the scenes, Termlike performs glyph rendering by utilizing what is commonly known as *sprite batching*. This is a very GPU-efficient technique that lets a program make as few draw calls as possible.

**Command buffering**

Whenever a program calls any of the `term_print` functions, Termlike does not immediately render the character glyphs. Instead, the call is buffered as a *command* that contains all the necessary information to make the glyph appear as expected.

Termlike will accumulate all these commands for a single frame and trigger them only when appropriate (and in the expected order). This allows a program to not worry about when or where it issues print commands, and is a requirement to guarantee correct layering/ordering when coupled with sprite batching (without it, glyphs on top could be flushed before those below).

## License

Termlike is a Free Open-Source Software project released under the [MIT License](LICENSE).
