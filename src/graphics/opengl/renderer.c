#include "../renderer.h" // graphics_*
#include "../viewport.h" // viewport, viewport_clip

#include "../../resources/cp437.h" // CP437, CP437_LENGTH

#include "renderable.h" // renderable, vector2, vector3
#include "glyph_renderer.h" // glyph_renderer, glyphs_*

#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, free
#include <stdbool.h> // bool
#include <stddef.h> // NULL
#include <string.h> // memcpy
#include <stdint.h> // int16_t, int32_t, uint16_t uint32_t

#include <gl3w/GL/gl3w.h> // gl*, GL*

#ifdef DEBUG
 #include <assert.h> // assert
#endif

struct frame_renderable {
    struct renderable renderable;
    GLuint texture_id;
    GLuint framebuffer;
    GLuint renderbuffer;
};

struct frame_vertex {
    int16_t x, y;
    struct texture texture;
};

struct glyph_uv {
    struct texture min;
    struct texture max;
};

struct graphics_shared {
    struct glyph_uv glyph_uvs[256];
    struct glyph_vertex glyph_vertices[GLYPH_VERTEX_COUNT];
    struct graphics_scale glyph;
    struct graphics_scale glyph_half;
};

struct graphics_context {
    struct graphics_shared shared;
    struct glyph_renderer * glyphs;
    struct frame_renderable screen;
    struct graphics_font font;
    struct viewport viewport;
    struct viewport_clip clip;
    struct color clear;
    struct color bars;
    GLuint font_texture_id;
};

static uint32_t cache_previous_code = 0;
static uint32_t cache_previous_index = 0;

static void graphics_process_errors(void);

static void graphics_setup(struct graphics_context *);
static void graphics_teardown(struct graphics_context *);

static void graphics_setup_screen_shader(struct graphics_context *);
static void graphics_setup_screen_texture(struct graphics_context *);
static void graphics_setup_screen_buffer(struct graphics_context *);
static void graphics_setup_screen_vbo(struct graphics_context *);

static void graphics_create_texture(struct graphics_image, GLuint * texture_id);

static uint32_t graphics_get_table_index(uint32_t code);

static void graphics_set_position(struct glyph_vertex (*)[GLYPH_VERTEX_COUNT],
                                  struct graphics_scale halved_glyph);

static void graphics_set_uv(struct glyph_vertex (*)[GLYPH_VERTEX_COUNT],
                            struct glyph_uv);

static void graphics_set_tint(struct glyph_vertex (*)[GLYPH_VERTEX_COUNT],
                              struct graphics_color);

static void graphics_get_transform(struct graphics_transform,
                                   struct viewport,
                                   struct graphics_scale glyph_size,
                                   struct glyph_transform *);

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
graphics_begin(struct graphics_context * const context)
{
    int32_t const width = context->viewport.resolution.width;
    int32_t const height = context->viewport.resolution.height;
    
    glBindFramebuffer(GL_FRAMEBUFFER, context->screen.framebuffer); {
        glViewport(0, 0, width, height);
        
        glClearColor(context->clear.r / 255.0f,
                     context->clear.g / 255.0f,
                     context->clear.b / 255.0f,
                     context->clear.a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    glyphs_begin(context->glyphs);
}

void
graphics_end(struct graphics_context * const context)
{
    glyphs_end(context->glyphs);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); {
        glViewport(context->clip.x,
                   context->clip.y,
                   context->clip.area.width,
                   context->clip.area.height);

        glClearColor(context->bars.r / 255.0f,
                     context->bars.g / 255.0f,
                     context->bars.b / 255.0f,
                     context->bars.a / 255.0f);
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
              struct graphics_transform const transform,
              uint32_t const code)
{
    bool const use_cache = code == cache_previous_code;
    
    uint32_t const table_index = use_cache ?
        cache_previous_index : graphics_get_table_index(code);
    
    cache_previous_code = code;
    cache_previous_index = table_index;
    
    struct glyph_vertex vertices[GLYPH_VERTEX_COUNT];
    struct glyph_uv const uv = context->shared.glyph_uvs[table_index];
    
    memcpy(&vertices, context->shared.glyph_vertices,
           sizeof(context->shared.glyph_vertices));
    
    graphics_set_uv(&vertices, uv);
    graphics_set_tint(&vertices, color);
    
    struct glyph_transform glyph_transform;
    
    graphics_get_transform(transform,
                           context->viewport,
                           context->shared.glyph,
                           &glyph_transform);
    
    glyph_transform.offset = (struct vector2) {
        .x = context->shared.glyph_half.horizontal,
        .y = context->shared.glyph_half.vertical
    };
    
    glyphs_add(context->glyphs, &vertices, glyph_transform,
               context->font_texture_id);
}

void
graphics_get_font(struct graphics_context const * const context,
                  struct graphics_font * const font)
{
    *font = context->font;
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
    
    // store commonly used values to avoid calculating over and over
    struct graphics_scale const glyph = (struct graphics_scale) {
        .horizontal = font.size,
        .vertical = font.size
    };
    
    struct graphics_scale const texture = (struct graphics_scale) {
        .horizontal = font.columns * glyph.horizontal,
        .vertical = font.rows * glyph.vertical
    };
    
    context->shared.glyph = glyph;
    context->shared.glyph_half = (struct graphics_scale) {
        .horizontal = glyph.horizontal / 2.0f,
        .vertical = glyph.vertical / 2.0f,
    };
    
    float const half_pixel = 0.5f;
    
    float const w = half_pixel / texture.horizontal;
    float const h = half_pixel / texture.vertical;
    
    for (uint16_t column = 0; column < 16; column++) {
        for (uint16_t row = 0; row < 16; row++) {
            struct vector2 source;
            
            source.x = column * glyph.horizontal;
            source.y = row * glyph.vertical;
            
            // flip it
            source.y = (texture.vertical - glyph.vertical) - source.y;
            
            struct vector2 const min = {
                .x = (source.x + w) / texture.horizontal,
                .y = (source.y + h) / texture.vertical
            };
            
            struct vector2 const max = {
                .x = (source.x - w + glyph.horizontal) / texture.horizontal,
                .y = (source.y - h + glyph.vertical) / texture.vertical
            };
            
            uint16_t const index = (row * 16) + column;
            
            // normalize to fixed-point values
            context->shared.glyph_uvs[index] = (struct glyph_uv) {
                .min = (struct texture) {
                    .u = (uint16_t)(UINT16_MAX * min.x),
                    .v = (uint16_t)(UINT16_MAX * min.y)
                },
                .max = (struct texture) {
                    .u = (uint16_t)(UINT16_MAX * max.x),
                    .v = (uint16_t)(UINT16_MAX * max.y)
                }
            };
        }
    }
    
    graphics_set_position(&context->shared.glyph_vertices,
                          context->shared.glyph_half);
}

void
graphics_invalidate(struct graphics_context * const context,
                    struct viewport const viewport)
{
    viewport_box(viewport, &context->viewport);
    viewport_clip(context->viewport, &context->clip);
    
    glyphs_invalidate(context->glyphs, context->viewport);
}

void
graphics_get_viewport(struct graphics_context const * const context,
                      struct viewport * const viewport)
{
    *viewport = context->viewport;
}

static
void
graphics_setup(struct graphics_context * const context)
{
    glClearDepth(1.0);
    
    graphics_setup_screen_shader(context);
    graphics_setup_screen_texture(context);
    graphics_setup_screen_buffer(context);
    graphics_setup_screen_vbo(context);
    
    context->clear = (struct color) {
        .r = 0,
        .g = 0,
        .b = 0,
        .a = 255
    };
    
    context->bars = (struct color) {
        .r = 30,
        .g = 30,
        .b = 30,
        .a = 255
    };
    
    context->font.columns = 0;
    context->font.rows = 0;
    context->font_texture_id = 0;
    
    context->glyphs = glyphs_init(context->viewport);
}

static
void
graphics_teardown(struct graphics_context * const context)
{
    glyphs_release(context->glyphs);
    
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
            fprintf(stderr, "OpenGL: screen framebuffer is invalid\n");
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static
void
graphics_setup_screen_vbo(struct graphics_context * const context)
{
    static struct frame_vertex const vertices[4] = {
        { .x = -INT16_MAX, .y = -INT16_MAX, .texture = { 0, 0 } },
        { .x = -INT16_MAX, .y =  INT16_MAX, .texture = { 0, UINT16_MAX } },
        { .x =  INT16_MAX, .y = -INT16_MAX, .texture = { UINT16_MAX, 0 } },
        { .x =  INT16_MAX, .y =  INT16_MAX, .texture = { UINT16_MAX, UINT16_MAX } },
    };
    
    glGenBuffers(1, &context->screen.renderable.vbo);
    glGenVertexArrays(1, &context->screen.renderable.vao);
    
    glBindVertexArray(context->screen.renderable.vao); {
        glBindBuffer(GL_ARRAY_BUFFER, context->screen.renderable.vbo); {
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                         GL_STATIC_DRAW);
            
            GLsizei const stride = sizeof(struct frame_vertex);
            
            glVertexAttribPointer(0,
                                  2,
                                  GL_SHORT, GL_TRUE,
                                  stride,
                                  0);
            glEnableVertexAttribArray(0);
            
            glVertexAttribPointer(1,
                                  2,
                                  GL_UNSIGNED_SHORT, GL_TRUE,
                                  stride,
                                  (GLvoid *)(sizeof(int16_t) * 2));
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

static
void
graphics_set_position(struct glyph_vertex (* const verts)[GLYPH_VERTEX_COUNT],
                      struct graphics_scale const halved_glyph)
{
    float const l = -halved_glyph.horizontal;
    float const r = halved_glyph.horizontal;
    float const b = -halved_glyph.vertical;
    float const t = halved_glyph.vertical;
    
    struct vector2 const bl = { .x = l, .y = b };
    struct vector2 const tl = { .x = l, .y = t };
    struct vector2 const tr = { .x = r, .y = t };
    struct vector2 const br = { .x = r, .y = b };
    
    (*verts)[0].position = (struct vector3) { .x = tl.x, .y = tl.y, .z = 0 };
    (*verts)[1].position = (struct vector3) { .x = br.x, .y = br.y, .z = 0 };
    (*verts)[2].position = (struct vector3) { .x = bl.x, .y = bl.y, .z = 0 };
    
    (*verts)[3].position = (struct vector3) { .x = tl.x, .y = tl.y, .z = 0 };
    (*verts)[4].position = (struct vector3) { .x = tr.x, .y = tr.y, .z = 0 };
    (*verts)[5].position = (struct vector3) { .x = br.x, .y = br.y, .z = 0 };
}

static
void
graphics_set_uv(struct glyph_vertex (* const verts)[GLYPH_VERTEX_COUNT],
                struct glyph_uv const uv)
{
    struct texture const bl = uv.min;
    struct texture const tr = uv.max;
    
    struct texture const tl = {
        .u = uv.min.u,
        .v = uv.max.v
    };
    
    struct texture const br = {
        .u = uv.max.u,
        .v = uv.min.v
    };
    
    (*verts)[0].texture = tl;
    (*verts)[1].texture = br;
    (*verts)[2].texture = bl;
    (*verts)[3].texture = tl;
    (*verts)[4].texture = tr;
    (*verts)[5].texture = br;
}

static
void
graphics_set_tint(struct glyph_vertex (* const verts)[GLYPH_VERTEX_COUNT],
                  struct graphics_color const color)
{
#ifdef DEBUG
    assert(sizeof(struct color) == sizeof(struct graphics_color));
#endif
    
    for (uint16_t i = 0; i < GLYPH_VERTEX_COUNT; i++) {
        memcpy(&(*verts)[i].color, &color, sizeof(struct color));
    }
}

static
void
graphics_get_transform(struct graphics_transform const transform,
                       struct viewport const viewport,
                       struct graphics_scale const glyph_size,
                       struct glyph_transform * const glyph)
{
    float const w = glyph_size.horizontal * transform.scale.horizontal;
    float const h = glyph_size.vertical * transform.scale.vertical;
    
    float const dx = (w - glyph_size.horizontal) / 2.0f;
    float const dy = (h - glyph_size.vertical) / 2.0f;
    
    float const x = transform.position.x + dx;
    float const y = transform.position.y + dy;
    
    float const flipped_y = (viewport.resolution.height -
                             glyph_size.vertical -
                             y);
    
    glyph->origin = (struct vector3) {
        .x = x,
        .y = flipped_y,
        .z = transform.position.z
    };
    
    glyph->angle = transform.angle;
    glyph->scale.x = transform.scale.horizontal;
    glyph->scale.y = transform.scale.vertical;
}

static
uint32_t
graphics_get_table_index(uint32_t const code)
{
    if (code >= 33 &&
        code <= 126) {
        // within basic ASCII range; skip mapping
        return code;
    }
    
    for (uint16_t i = 0; i < 33; i++) {
        if (CP437[i] == code) {
            return i;
        }
    }
    
    for (uint16_t i = 127; i < CP437_LENGTH; i++) {
        if (CP437[i] == code) {
            return i;
        }
    }
    
    return 63; // mapping not found, default to '?'
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
        
        fprintf(stderr, "OpenGL: shader compilation error: %s\n", (char *)log);
        
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
        
        fprintf(stderr, "OpenGL: program link error: %s\n", (char *)log);
        
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
        fprintf(stderr, "OpenGL: %d\n", error);
    }
}
