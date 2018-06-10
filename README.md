# Termlike

[![code style: compliant](https://img.shields.io/badge/code%20style-compliant-000000.svg)](https://github.com/jhauberg/comply)

A low-profile library for building high-performance and cross-platform games in the aesthetic of traditional text-based games, but with the benefits of modern graphics.

The name *Termlike* is in reference to the [*Roguelike*](https://en.wikipedia.org/wiki/Roguelike) genre of games, which historically has roots in text-based terminals.

Termlike is written entirely in **C99** and requires **OpenGL 3.3 (Core Profile)**.

## Dependencies

A few dependencies are required to keep the scope of the project down. Most of these are small and already included as part of the repository (see [external](/external)).

**Requires installation:**

- [`GLFW3`](https://github.com/glfw/glfw) handles **cross-platform window creation**

**Included:**

- [`gl3w`](https://github.com/skaslev/gl3w) for **OpenGL Core Profile** header loading

## Usage

Termlike is intended as a static library (`libterm.a`) that you link into your program.

## Build

This project is in very early stages of development. For the time being, build instructions are not included.

## License

Termlike is a Free Open-Source Software project released under the [MIT License](LICENSE).
