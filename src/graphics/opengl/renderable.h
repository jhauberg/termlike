#pragma once

#include <gl3w/GL/gl3w.h> // GLuint, GLenum, GLchar

#include <stdint.h> // uint8_t, uint16_t

struct vector2 {
    float x, y;
};

struct vector3 {
    float x, y, z;
};

struct color {
    uint8_t r, g, b, a;
};

struct texture {
    uint16_t u, v;
};

struct renderable {
    GLuint program;
    GLuint vbo;
    GLuint vao;
};

struct glyph_vertex {
    struct vector3 position; // 12
    struct color color; // 4
    struct texture texture; // 4 = 20
};

GLuint graphics_compile_shader(GLenum type, GLchar const * source);
GLuint graphics_link_program(GLuint vs, GLuint fs);
