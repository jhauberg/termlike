#pragma once

#include <gl3w/GL/gl3w.h> // GLuint

struct vector2 {
    float x, y;
};

struct vector3 {
    float x, y, z;
};

struct color {
    float r, g, b, a;
};

struct renderable {
    GLuint program;
    GLuint vbo;
    GLuint vao;
};

struct glyph_vertex {
    struct vector3 position;
    struct color color;
    struct vector2 texture_coord;
};

GLuint graphics_compile_shader(GLenum type, GLchar const * source);
GLuint graphics_link_program(GLuint vs, GLuint fs);
