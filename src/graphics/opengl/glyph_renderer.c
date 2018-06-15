#include "glyph_renderer.h" // glyph_renderer, glyphs_*

#include <stdlib.h> // malloc, free, qsort

#include "../viewport.h" // viewport

#ifdef DEBUG
 #include "../../platform/profiler.h" // profiler_*
#endif

#include "renderable.h" // glyph_vertex, vector3

#include <gl3w/GL/gl3w.h> // gl*, GL*
#include <linmath/linmath.h> // mat4x4, mat4x4_*

#define MAX_GLYPHS 2048 // flush when reaching this limit

#define GLYPH_VERTEX_COUNT (2 * 3) // 2 triangles per quad = 6 vertices
#define GLYPH_BATCH_VERTEX_COUNT (MAX_GLYPHS * GLYPH_VERTEX_COUNT)

static int32_t glyph_compare(void const * glyph, void const * other_glyph);

struct glyph_renderer {
    struct glyph_vertex vertices[GLYPH_BATCH_VERTEX_COUNT];
    mat4x4 transform;
    struct renderable renderable;
    GLuint current_texture_id;
    uint16_t count;
};

static void glyphs_state(bool enable);
static void glyphs_draw(struct glyph_renderer const *);

struct glyph_renderer *
glyphs_init(struct viewport const viewport)
{
    struct glyph_renderer * batch = malloc(sizeof(struct glyph_renderer));
    
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
    "uniform int enable_additive_tint;\n"
    "uniform sampler2D sampler;\n"
    "layout (location = 0) out vec4 fragment_color;\n"
    "void main() {\n"
    "    vec4 sampled_color = texture(sampler, texture_coord.st);\n"
    "    if (enable_additive_tint != 0) {\n"
    "        fragment_color = (sampled_color + vec4(tint.rgb, 0)) * tint.a;\n"
    "    } else {\n"
    "        fragment_color = sampled_color * tint;\n"
    "    }\n"
    "}";

    GLuint const vs = graphics_compile_shader(GL_VERTEX_SHADER,
                                            vertex_shader);
    GLuint const fs = graphics_compile_shader(GL_FRAGMENT_SHADER,
                                            fragment_shader);
    
    batch->renderable.program = graphics_link_program(vs, fs);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    glGenBuffers(1, &batch->renderable.vbo);
    glGenVertexArrays(1, &batch->renderable.vao);
    
    glBindVertexArray(batch->renderable.vao); {
        glBindBuffer(GL_ARRAY_BUFFER, batch->renderable.vbo); {
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
                                  GL_FLOAT, GL_FALSE,
                                  stride,
                                  (GLvoid *)(sizeof(struct vector3)));
            glEnableVertexAttribArray(1);
            
            glVertexAttribPointer(2,
                                  2,
                                  GL_FLOAT, GL_FALSE,
                                  stride,
                                  (GLvoid *)(sizeof(struct vector3) +
                                             sizeof(struct color)));
            glEnableVertexAttribArray(2);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);
    
    glyphs_invalidate(batch, viewport);
    
    return batch;
}

void
glyphs_release(struct glyph_renderer * const batch)
{
    glDeleteProgram(batch->renderable.program);
    glDeleteVertexArrays(1, &batch->renderable.vao);
    glDeleteBuffers(1, &batch->renderable.vbo);
    
    free(batch);
}

void
glyphs_invalidate(struct glyph_renderer * const batch,
                  struct viewport const viewport)
{
    mat4x4 model;
    mat4x4_identity(model);

    mat4x4 view;
    mat4x4_identity(view);
    
    mat4x4 projection;
    mat4x4_identity(projection);
    
    mat4x4_ortho(projection,
                 0, viewport.resolution.width,
                 0, viewport.resolution.height,
                 -1,  // near
                 1); // far
    
    mat4x4 model_view;
    mat4x4_mul(model_view, model, view);
    mat4x4_mul(batch->transform, projection, model_view);
    
    glyphs_reset(batch);
}

void
glyphs_add(struct glyph_renderer * const batch,
           struct glyph_vertex const * const vertices,
           struct vector3 const origin,
           float const angle,
           GLuint const texture_id)
{
    if (batch->current_texture_id != 0 &&
        batch->current_texture_id != texture_id) {
        if (glyphs_flush(batch)) {
            // texture switch requires flushing (don't flush if not set yet)
        }
    }
    
    batch->current_texture_id = texture_id;
    
    if (batch->count + 1 > MAX_GLYPHS) {
        if (glyphs_flush(batch)) {
            // hitting capacity requires flushing
        }
    }
    
    uint32_t const offset = batch->count * GLYPH_VERTEX_COUNT;
    
    float const scale = 1;
    
    mat4x4 translated;
    mat4x4_translate(translated, origin.x, origin.y, origin.z);
    
    mat4x4 rotated;
    mat4x4_identity(rotated);
    mat4x4_rotate_Z(rotated, rotated, angle);
    
    mat4x4 scaled;
    mat4x4_identity(scaled);
    mat4x4_scale_aniso(scaled, scaled, scale, scale, scale);
    
    mat4x4 transform;
    mat4x4_mul(transform, translated, rotated);
    mat4x4_mul(transform, transform, scaled);
    
    for (uint16_t i = 0; i < GLYPH_VERTEX_COUNT; i++) {
        struct glyph_vertex vertex = vertices[i];
        
        vec4 position = {
            vertex.position.x,
            vertex.position.y,
            vertex.position.z, 1
        };
        
        vec4 world_position;
        
        mat4x4_mul_vec4(world_position, transform, position);
        
        vertex.position = (struct vector3) {
            world_position[0],
            world_position[1],
            world_position[2]
        };
        
        batch->vertices[offset + i] = vertex;
    }
    
    batch->count += 1;
}

bool
glyphs_flush(struct glyph_renderer * const batch)
{
    if (batch->count == 0) {
        return false;
    }
    
    qsort(&batch->vertices, batch->count,
          sizeof(struct glyph_vertex) * GLYPH_VERTEX_COUNT,
          glyph_compare);
    
    glyphs_draw(batch);
    
    batch->count = 0;
    
    return true;
}

void
glyphs_reset(struct glyph_renderer * const batch)
{
    batch->count = 0;
    batch->current_texture_id = 0;
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
glyphs_draw(struct glyph_renderer const * const batch)
{
    glyphs_state(true);
    
    glUseProgram(batch->renderable.program);
    
    // note that this flag will affect entire batch
    GLint const uniform_tint =
    glGetUniformLocation(batch->renderable.program,
                         "enable_additive_tint");
    GLint const uniform_transform =
    glGetUniformLocation(batch->renderable.program,
                         "transform");
    
    glUniform1i(uniform_tint, false);
    glUniformMatrix4fv(uniform_transform, 1, GL_FALSE, *batch->transform);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, batch->current_texture_id); {
        glBindVertexArray(batch->renderable.vao); {
            glBindBuffer(GL_ARRAY_BUFFER, batch->renderable.vbo); {
                GLsizei const count = batch->count * GLYPH_VERTEX_COUNT;
                
                GLsizeiptr const size =
                sizeof(struct glyph_vertex) * (uint32_t)count;
                
                glBufferSubData(GL_ARRAY_BUFFER,
                                0,
                                size,
                                batch->vertices);
                glDrawArrays(GL_TRIANGLES, 0, count);
#ifdef DEBUG
                profiler_increment_draw_count(1);
#endif
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindVertexArray(0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    
    glyphs_state(false);
}

static
int32_t
glyph_compare(void const * const glyph,
              void const * const other_glyph)
{
    struct glyph_vertex const * lhs = (struct glyph_vertex *)glyph;
    struct glyph_vertex const * rhs = (struct glyph_vertex *)other_glyph;
    
    if (lhs->position.z < rhs->position.z) {
        return -1;
    } else if (lhs->position.z > rhs->position.z) {
        return 1;
    }
    
    return 0;
}
