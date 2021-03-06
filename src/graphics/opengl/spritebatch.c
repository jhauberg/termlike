#include <termlike/graphics/viewport.h> // viewport

#ifdef TERM_INCLUDE_PROFILER
 #include <termlike/platform/profiler.h> // profiler_*
#endif

#include "spritebatch.h" // glyph_renderer, glyphs_*

#include <stdbool.h> // bool
#include <stdlib.h> // malloc, free
#include <stddef.h> // NULL

#include "renderable.h" // glyph_vertex, vector3

#include <gl3w/GL/gl3w.h> // gl*, GL*
#include <linmath/linmath.h> // mat4x4, mat4x4_*

#define MAX_GLYPHS 4096 // flush when reaching this limit

#define GLYPH_BATCH_VERTEX_COUNT (MAX_GLYPHS * GLYPH_VERTEX_COUNT)

struct glyph_batch {
    struct glyph_vertex vertices[GLYPH_BATCH_VERTEX_COUNT];
    uint32_t count;
};

struct glyph_renderer {
    struct glyph_batch batch;
    struct renderable renderable;
    mat4x4 transform;
    GLuint current_texture_id;
};

static void glyphs_state(bool enable);
static bool glyphs_flush(struct glyph_renderer *);
static void glyphs_reset(struct glyph_renderer *);
static void glyphs_draw(struct glyph_renderer const *);

struct glyph_renderer *
glyphs_init(struct viewport const viewport)
{
    struct glyph_renderer * renderer = malloc(sizeof(struct glyph_renderer));

    char const * const vertex_shader =
    "#version 330 core\n"
    "layout (location = 0) in vec3 vertex_position;\n"
    "layout (location = 1) in vec4 vertex_color;\n"
    "layout (location = 2) in vec2 vertex_texture_coord;\n"
    "uniform mat4 transform;\n"
    "out vec2 texture_coord;\n"
    "out vec4 tint;\n"
    "void main() {\n"
    "    gl_Position = transform * vec4(vertex_position, 1);\n"
    "    texture_coord = vertex_texture_coord.st;\n"
    "    tint = vertex_color;\n"
    "}\n";

    char const * const fragment_shader =
    "#version 330 core\n"
    "in vec2 texture_coord;\n"
    "in vec4 tint;\n"
    "uniform sampler2D sampler;\n"
    "layout (location = 0) out vec4 fragment_color;\n"
    "void main() {\n"
    "    fragment_color = texture(sampler, texture_coord.st) * tint;\n"
    "}";

    GLuint const vs = graphics_compile_shader(GL_VERTEX_SHADER,
                                              vertex_shader);
    GLuint const fs = graphics_compile_shader(GL_FRAGMENT_SHADER,
                                              fragment_shader);

    renderer->renderable.program = graphics_link_program(vs, fs);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenBuffers(1, &renderer->renderable.vbo);
    glGenVertexArrays(1, &renderer->renderable.vao);

    glBindVertexArray(renderer->renderable.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->renderable.vbo);

    GLsizei const stride = sizeof(struct glyph_vertex);

    glBufferData(GL_ARRAY_BUFFER,
                 GLYPH_BATCH_VERTEX_COUNT * stride,
                 NULL,
                 GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT, GL_FALSE,
                          stride,
                          0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1,
                          4,
                          GL_UNSIGNED_BYTE, GL_TRUE,
                          stride,
                          (GLvoid *)(sizeof(struct vector3)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2,
                          2,
                          GL_UNSIGNED_SHORT, GL_TRUE,
                          stride,
                          (GLvoid *)(sizeof(struct vector3) +
                                     sizeof(struct color)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glyphs_invalidate(renderer, viewport);

    return renderer;
}

void
glyphs_release(struct glyph_renderer * const renderer)
{
    glDeleteProgram(renderer->renderable.program);
    glDeleteVertexArrays(1, &renderer->renderable.vao);
    glDeleteBuffers(1, &renderer->renderable.vbo);

    free(renderer);
}

void
glyphs_invalidate(struct glyph_renderer * const renderer,
                  struct viewport const viewport)
{
    mat4x4 model;
    mat4x4_identity(model);

    mat4x4 view;
    mat4x4_identity(view);

    mat4x4 projection;
    mat4x4_identity(projection);

    mat4x4_ortho(projection,
                 0, (float)viewport.resolution.width,
                 0, (float)viewport.resolution.height,
                 -1,  // near
                 1); // far

    mat4x4 model_view;
    mat4x4_mul(model_view, model, view);
    mat4x4_mul(renderer->transform, projection, model_view);

    glyphs_reset(renderer);
}

void
glyphs_add(struct glyph_renderer * const renderer,
           struct glyph_vertex (* const vertices)[GLYPH_VERTEX_COUNT],
           struct glyph_transform const transform,
           GLuint const texture_id)
{
    if (renderer->current_texture_id != 0 &&
        renderer->current_texture_id != texture_id) {
        if (glyphs_flush(renderer)) {
            // texture switch requires flushing (don't flush if not set yet)
        }
    }

    renderer->current_texture_id = texture_id;

    if (renderer->batch.count + 1 > MAX_GLYPHS) {
        if (glyphs_flush(renderer)) {
            // hitting capacity requires flushing
        }
    }

    bool const requires_scaling = (transform.scale.x > 1 ||
                                   transform.scale.x < 1 ||
                                   transform.scale.y > 1 ||
                                   transform.scale.y < 1);

    bool const requires_rotation = (transform.angle > 0 ||
                                    transform.angle < 0);

    bool const requires_transformation = (requires_scaling ||
                                          requires_rotation);

    uint32_t const vertex_offset = renderer->batch.count * GLYPH_VERTEX_COUNT;

    mat4x4 transformed;

    if (requires_transformation) {
        mat4x4 translated;
        mat4x4_identity(translated);
        mat4x4_translate(translated,
                         transform.origin.x,
                         transform.origin.y,
                         transform.origin.z);

        mat4x4 rotated;
        mat4x4_identity(rotated);

        if (requires_rotation) {
            mat4x4_rotate_Z(rotated, rotated,
                            transform.angle);
        }

        mat4x4 scaled;
        mat4x4_identity(scaled);

        if (requires_scaling) {
            mat4x4_scale_aniso(scaled, scaled,
                               transform.scale.x,
                               transform.scale.y,
                               1);
        }

        mat4x4_mul(transformed, translated, rotated);
        mat4x4_mul(transformed, transformed, scaled);
    }

    for (uint16_t i = 0; i < GLYPH_VERTEX_COUNT; i++) {
        struct glyph_vertex vertex = (*vertices)[i];

        if (requires_transformation) {
            vec4 position = {
                vertex.position.x,
                vertex.position.y,
                vertex.position.z,
                1
            };

            vec4 world_position;

            mat4x4_mul_vec4(world_position, transformed, position);

            vertex.position = (struct vector3) {
                world_position[0] + transform.offset.x,
                world_position[1] + transform.offset.y,
                world_position[2]
            };
        } else {
            vertex.position = (struct vector3) {
                vertex.position.x + transform.origin.x + transform.offset.x,
                vertex.position.y + transform.origin.y + transform.offset.y,
                vertex.position.z + transform.origin.z
            };
        }

        renderer->batch.vertices[vertex_offset + i] = vertex;
    }

    renderer->batch.count += 1;
}

static
bool
glyphs_flush(struct glyph_renderer * const renderer)
{
    if (renderer->batch.count == 0) {
        return false;
    }

    glyphs_draw(renderer);

    renderer->batch.count = 0;

    return true;
}

static
void
glyphs_reset(struct glyph_renderer * const renderer)
{
    renderer->batch.count = 0;
    renderer->current_texture_id = 0;
}

void
glyphs_begin(struct glyph_renderer const * const renderer)
{
    glyphs_state(true);

    glUseProgram(renderer->renderable.program);

    GLint const uniform_transform =
    glGetUniformLocation(renderer->renderable.program, "transform");
    glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, *renderer->transform);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderer->current_texture_id);
    glBindVertexArray(renderer->renderable.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->renderable.vbo);
}

void
glyphs_end(struct glyph_renderer * const renderer)
{
    glyphs_flush(renderer);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    glyphs_state(false);
}

static
void
glyphs_state(bool const enable)
{
    if (enable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
    }
}

static
void
glyphs_draw(struct glyph_renderer const * const renderer)
{
    uint32_t const count = renderer->batch.count * GLYPH_VERTEX_COUNT;

    GLsizeiptr const size = sizeof(struct glyph_vertex) * count;

    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    size,
                    renderer->batch.vertices);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)count);
#ifdef TERM_INCLUDE_PROFILER
    profiler_increment_draw_count(1);
#endif
}

