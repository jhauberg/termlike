#include "../renderer.h" // graphics_*
#include "../viewport.h" // viewport, viewport_clip

#include "renderable.h" // renderable, vector2, vector3
#include "glyph_renderer.h" // glyph_renderer, glyphs_*

#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, free

#include <gl3w/GL/gl3w.h> // gl*, GL*

struct frame_renderable {
    struct renderable renderable;
    GLuint texture_id;
    GLuint framebuffer;
    GLuint renderbuffer;
};

struct frame_vertex {
    struct vector3 position;
    struct vector2 texture_coord;
};

struct graphics_context {
    struct glyph_renderer * batch;
    struct frame_renderable screen;
    struct graphics_font font;
    struct color clear;
    struct color bars;
    struct viewport viewport;
    GLuint font_texture_id;
};

static void graphics_process_errors(void);

static void graphics_setup(struct graphics_context *);
static void graphics_teardown(struct graphics_context *);

static void graphics_setup_screen_shader(struct graphics_context *);
static void graphics_setup_screen_texture(struct graphics_context *);
static void graphics_setup_screen_buffer(struct graphics_context *);
static void graphics_setup_screen_vbo(struct graphics_context *);

static void graphics_create_texture(struct graphics_image, GLuint * texture_id);

struct graphics_context *
graphics_init(struct viewport const viewport)
{
    struct graphics_context * context = malloc(sizeof(struct graphics_context));
    
    context->viewport = viewport;
    
    graphics_setup(context);
    
    return context;
}

void
graphics_release(struct graphics_context * const context)
{
    graphics_teardown(context);
    
    free(context);
}

void
graphics_begin(struct graphics_context const * const context)
{
    int32_t const width = context->viewport.resolution.width;
    int32_t const height = context->viewport.resolution.height;
    
    glBindFramebuffer(GL_FRAMEBUFFER, context->screen.framebuffer); {
        glViewport(0, 0, width, height);
        
        glClearColor(context->clear.r, context->clear.g, context->clear.b,
                     context->clear.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void
graphics_end(struct graphics_context * const context)
{
    glyphs_flush(context->batch);
    
    struct viewport_clip clip = viewport_get_clip(context->viewport);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); {
        glViewport(clip.x, clip.y, clip.area.width, clip.area.height);

        glClearColor(context->bars.r, context->bars.g, context->bars.b,
                     context->bars.a);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(context->screen.renderable.program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, context->screen.texture_id);
        glBindVertexArray(context->screen.renderable.vao); {
            glBindBuffer(GL_ARRAY_BUFFER, context->screen.renderable.vbo); {
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUseProgram(0);
    }
    
    graphics_process_errors();
}

void
graphics_draw(struct graphics_context const * const context,
              struct graphics_color const color,
              struct graphics_position const position,
              float const angle,
              float const scale,
              uint32_t const code)
{
    int32_t const table_size = context->font.columns * context->font.rows;
    
    int32_t table_index = -1;
    
    if (context->font.codepage != NULL) {
        for (int32_t i = 0; i < table_size; i++) {
            if (context->font.codepage[i] == code) {
                table_index = i;
                
                break;
            }
        }
    }
    
    if (table_index < 0 ||
        table_index > table_size) {
        table_index = -1;
    }
    
    if (table_index == -1) {
        return;
    }
    
    int32_t const character_row = table_index / context->font.columns;
    int32_t const character_column = table_index % context->font.columns;
    
    float const texture_width = context->font.columns * context->font.size;
    float const texture_height = context->font.rows * context->font.size;
    
    struct vector2 source;
    
    source.x = character_column * context->font.size;
    source.y = character_row * context->font.size;
    // flip it
    source.y = (texture_height - context->font.size) - source.y;

    struct glyph_vertex vertices[6];
    
    float const half = context->font.size / 2.0f;
    
    float const l = -half;
    float const r = half;
    float const b = -half;
    float const t = half;
    
    struct vector2 const bl = { .x = l, .y = b };
    struct vector2 const tl = { .x = l, .y = t };
    struct vector2 const tr = { .x = r, .y = t };
    struct vector2 const br = { .x = r, .y = b };
    
    vertices[0].position = (struct vector3) { .x = tl.x, .y = tl.y, .z = 0 };
    vertices[1].position = (struct vector3) { .x = br.x, .y = br.y, .z = 0 };
    vertices[2].position = (struct vector3) { .x = bl.x, .y = bl.y, .z = 0 };
    
    vertices[3].position = (struct vector3) { .x = tl.x, .y = tl.y, .z = 0 };
    vertices[4].position = (struct vector3) { .x = tr.x, .y = tr.y, .z = 0 };
    vertices[5].position = (struct vector3) { .x = br.x, .y = br.y, .z = 0 };
    
    struct vector2 const uv_min = {
        .x = source.x / texture_width,
        .y = source.y / texture_height
    };
    
    struct vector2 const uv_max = {
        .x = (source.x + context->font.size) / texture_width,
        .y = (source.y + context->font.size) / texture_height
    };
    
    struct vector2 const uv_bl = { .x = uv_min.x, .y = uv_min.y };
    struct vector2 const uv_tl = { .x = uv_min.x, .y = uv_max.y };
    struct vector2 const uv_tr = { .x = uv_max.x, .y = uv_max.y };
    struct vector2 const uv_br = { .x = uv_max.x, .y = uv_min.y };
    
    vertices[0].texture_coord = uv_tl;
    vertices[1].texture_coord = uv_br;
    vertices[2].texture_coord = uv_bl;
    
    vertices[3].texture_coord = uv_tl;
    vertices[4].texture_coord = uv_tr;
    vertices[5].texture_coord = uv_br;
    
    struct color tint = {
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a
    };
    
    vertices[0].color = tint;
    vertices[1].color = tint;
    vertices[2].color = tint;
    
    vertices[3].color = tint;
    vertices[4].color = tint;
    vertices[5].color = tint;
    
    struct viewport const viewport = graphics_get_viewport(context);
    
    float const flipped_y = (viewport.resolution.height -
                             context->font.size -
                             position.y);
    
    struct vector3 origin;
    
    origin.x = position.x + half;
    origin.y = flipped_y + half;
    origin.z = position.z;

    glyphs_add(context->batch,
               vertices,
               origin,
               angle,
               scale,
               context->font_texture_id);
}

void
graphics_set_font(struct graphics_context * const context,
                  struct graphics_image const image,
                  struct graphics_font const font)
{
    if (context->font_texture_id != 0) {
        glDeleteTextures(1, &context->font_texture_id);
    }
    
    graphics_create_texture(image, &context->font_texture_id);
    
    context->font = font;
}

void
graphics_invalidate(struct graphics_context * const context,
                    struct viewport const viewport)
{
    context->viewport = viewport_box(viewport);
    
    glyphs_invalidate(context->batch, context->viewport);
}

struct viewport
graphics_get_viewport(struct graphics_context const * const context)
{
    return context->viewport;
}

static
void
graphics_setup(struct graphics_context * const context)
{
    // clear depth should be set once and not changed afterwards
    glClearDepth(1.0);
    
    graphics_setup_screen_shader(context);
    graphics_setup_screen_texture(context);
    graphics_setup_screen_buffer(context);
    graphics_setup_screen_vbo(context);
    
    context->clear = (struct color) {
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 1
    };
    
    context->bars = (struct color) {
        .r = 0.1f,
        .g = 0.1f,
        .b = 0.1f,
        .a = 1
    };
    
    context->font.codepage = NULL;
    context->font.columns = 0;
    context->font.rows = 0;
    context->font_texture_id = 0;
    
    context->batch = glyphs_init(context->viewport);
}

static
void
graphics_teardown(struct graphics_context * const context)
{
    glyphs_release(context->batch);
    
    glDeleteTextures(1, &context->font_texture_id);
    glDeleteTextures(1, &context->screen.texture_id);
    glDeleteFramebuffers(1, &context->screen.framebuffer);
    glDeleteRenderbuffers(1, &context->screen.renderbuffer);
    glDeleteProgram(context->screen.renderable.program);
    glDeleteVertexArrays(1, &context->screen.renderable.vao);
    glDeleteBuffers(1, &context->screen.renderable.vbo);
}

static
void
graphics_setup_screen_shader(struct graphics_context * const context)
{
    char const * const vertex_shader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 vertex_position;\n"
    "layout (location = 1) in vec2 vertex_texture_coord;\n"
    "out vec2 texture_coord;\n"
    "void main() {\n"
    "    gl_Position = vec4(vertex_position, 1);\n"
    "    texture_coord = vertex_texture_coord.st;\n"
    "}\n";
    
    char const * const fragment_shader =
    "#version 330 core\n"
    "in vec2 texture_coord;\n"
    "uniform sampler2D sampler;\n"
    "layout (location = 0) out vec4 fragment_color;\n"
    "void main() {\n"
    "    vec4 sampled_color = texture(sampler, texture_coord.st);\n"
    "    fragment_color = sampled_color;\n"
    "}\n";
    
    GLuint const vs = graphics_compile_shader(GL_VERTEX_SHADER,
                                              vertex_shader);
    GLuint const fs = graphics_compile_shader(GL_FRAGMENT_SHADER,
                                              fragment_shader);
    
    context->screen.renderable.program = graphics_link_program(vs, fs);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
}

static
void
graphics_setup_screen_texture(struct graphics_context * const context)
{
    glGenTextures(1, &context->screen.texture_id);
    glBindTexture(GL_TEXTURE_2D, context->screen.texture_id); {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                     context->viewport.resolution.width,
                     context->viewport.resolution.height,
                     0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

static
void
graphics_setup_screen_buffer(struct graphics_context * const context)
{
    glGenFramebuffers(1, &context->screen.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, context->screen.framebuffer); {
        glGenRenderbuffers(1, &context->screen.renderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, context->screen.renderbuffer); {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                  context->viewport.resolution.width,
                                  context->viewport.resolution.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                      GL_DEPTH_ATTACHMENT,
                                      GL_RENDERBUFFER,
                                      context->screen.renderbuffer);
            glFramebufferTexture(GL_FRAMEBUFFER,
                                 GL_COLOR_ATTACHMENT0,
                                 context->screen.texture_id,
                                 0);
        }
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        
        GLenum const draw_buffers[1] = {
            GL_COLOR_ATTACHMENT0
        };
        
        glDrawBuffers(1, draw_buffers);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "OpenGL: screen framebuffer is invalid");
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static
void
graphics_setup_screen_vbo(struct graphics_context * const context)
{
    static struct frame_vertex const vertices[4] = {
        { .position = { -1, -1, 0 }, .texture_coord = { 0, 0 } },
        { .position = { -1,  1, 0 }, .texture_coord = { 0, 1 } },
        { .position = {  1, -1, 0 }, .texture_coord = { 1, 0 } },
        { .position = {  1,  1, 0 }, .texture_coord = { 1, 1 } },
    };
    
    glGenBuffers(1, &context->screen.renderable.vbo);
    glGenVertexArrays(1, &context->screen.renderable.vao);
    
    glBindVertexArray(context->screen.renderable.vao); {
        glBindBuffer(GL_ARRAY_BUFFER, context->screen.renderable.vbo); {
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            
            glVertexAttribPointer(0 /* position location */,
                                  3 /* components per vertex */,
                                  GL_FLOAT, GL_FALSE /* not normalized */,
                                  sizeof(struct frame_vertex),
                                  0);
            glEnableVertexAttribArray(0);
            
            glVertexAttribPointer(1 /* texture coord location */,
                                  2 /* components per vertex */,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(struct frame_vertex),
                                  (GLvoid *)(sizeof(struct vector3)));
            glEnableVertexAttribArray(1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
}

static
void
graphics_create_texture(struct graphics_image const image,
                        GLuint * const texture_id)
{
    GLenum format = 0;
    
    if (image.components == 3) {
        format = GL_RGB;
    } else if (image.components == 4) {
        format = GL_RGBA;
    }
    
    if (format == 0) {
        return;
    }
    
    GLint const format_internal = (GLint)format;
    
    glGenTextures(1, texture_id);
    glBindTexture(GL_TEXTURE_2D, *texture_id); {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glTexImage2D(GL_TEXTURE_2D,
                     0, format_internal,
                     image.width, image.height,
                     0, format,
                     GL_UNSIGNED_BYTE,
                     image.data);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint
graphics_compile_shader(GLenum const type,
                        GLchar const * const source)
{
    GLuint const shader = glCreateShader(type);
    
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint param;
    
    glGetShaderiv(shader, GL_COMPILE_STATUS, &param);
    
    if (param != GL_TRUE) {
        GLchar log[4096];
        
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        
        fprintf(stderr, "OpenGL: shader compilation error: %s", (char *)log);
        
        return 0;
    }
    
    return shader;
}

GLuint
graphics_link_program(GLuint const vs,
                      GLuint const fs)
{
    GLuint const program = glCreateProgram();
    
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    
    glLinkProgram(program);
    
    GLint param;
    
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    
    if (param != GL_TRUE) {
        GLchar log[4096];
        
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        
        fprintf(stderr, "OpenGL: program link error: %s", (char *)log);
        
        return 0;
    }
    
    return program;
}

static
void
graphics_process_errors(void)
{
    GLenum error;
    
    while ((error = glGetError()) != GL_NO_ERROR) {
        fprintf(stderr, "OpenGL: %d", error);
    }
}
