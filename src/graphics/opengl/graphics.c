#include "../graphics.h" // graphics_*
#include "../viewport.h" // viewport, viewport_clip

#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, free

#include <gl3w/GL/gl3w.h> // gl*, GL*

struct renderable {
    GLuint program;
    GLuint vbo;
    GLuint vao;
};

struct renderable_frame {
    struct renderable renderable;
    GLuint texture_id;
    GLuint framebuffer;
    GLuint renderbuffer;
};

struct vector2 {
    float x, y;
};

struct vector3 {
    float x, y, z;
};

struct vertex {
    struct vector3 position;
    struct vector2 texture_coord;
};

struct graphics_context {
    struct renderable_frame screen;
    struct viewport viewport;
    struct graphics_font font;
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

static GLuint graphics_compile_shader(GLenum type, GLchar const * source);
static GLuint graphics_link_program(GLuint vs, GLuint fs);

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
        
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void
graphics_end(struct graphics_context const * const context)
{
    struct viewport_clip clip = viewport_get_clip(context->viewport);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0); {
        glViewport(clip.x, clip.y, clip.area.width, clip.area.height);
        
        // note that we don't need to clear this framebuffer, as we're
        // expecting to overwrite every pixel every frame anyway (with the
        // opaque texture of the post framebuffer) however; if we wanted to
        // apply color to the letter/pillar-boxed bars, we could do that here
        // by clearing the color buffer
        
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
graphics_set_font(struct graphics_context * const context,
                  struct graphics_image const image,
                  struct graphics_font const font)
{
    graphics_create_texture(image, &context->font_texture_id);
    
    context->font = font;
}

void
graphics_invalidate(struct graphics_context * const context,
                    struct viewport const viewport)
{
    context->viewport = viewport_box(viewport);
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
    glClearColor(0, 0, 0, 1);
    // clear depth should be set once and not changed afterwards
    glClearDepth(1.0);
    
    graphics_setup_screen_shader(context);
    graphics_setup_screen_texture(context);
    graphics_setup_screen_buffer(context);
    graphics_setup_screen_vbo(context);
}

static
void
graphics_teardown(struct graphics_context * const context)
{
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
    static struct vertex const vertices[4] = {
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
                                  sizeof(struct vertex),
                                  0);
            glEnableVertexAttribArray(0);
            
            glVertexAttribPointer(1 /* texture coord location */,
                                  2 /* components per vertex */,
                                  GL_FLOAT, GL_FALSE,
                                  sizeof(struct vertex),
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

static
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

static
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
