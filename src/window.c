#include <termlike/input.h> // TERM_KEY_*

#include "state.h" // key_state
#include "window.h" // window_*

#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, free, NULL
#include <stdint.h> // int32_t
#include <stdbool.h> // bool

// use gl3w to load opengl headers
#include <gl3w/GL/gl3w.h> // *
// which means GLFW should not do that
#define GLFW_INCLUDE_NONE

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wdocumentation"
#endif

#include <GLFW/glfw3.h> // glfw*, GLFW*

#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

struct window_position {
    int32_t x;
    int32_t y;
};

struct window_context {
    /** A reference to the current window */
    GLFWwindow * window;
    /** Actual display size; e.g. operating resolution ⨉ pixel size */
    struct window_size display;
    /** Preferred window positioning; only used if initially fullscreen */
    struct window_position preferred_position;
};

static void window_callback_error(int32_t error, char const * description);
static void window_callback_size(GLFWwindow *, int32_t width, int32_t height);

static void window_make_windowed(GLFWwindow *,
                                 struct window_position,
                                 struct window_size);
static void window_make_fullscreen(GLFWwindow *,
                                   struct window_position * previous);

struct window_context *
window_create(struct window_params const params)
{
    glfwSetErrorCallback(window_callback_error);
    
    if (!glfwInit()) {
        fprintf(stderr, "GLFW (%s) failed to initialize",
                glfwGetVersionString());
        
        return NULL;
    }
    
    glfwWindowHint(GLFW_SAMPLES, 0);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
    
    GLFWwindow * window = NULL;
    
    struct window_size display = params.display;
    struct window_position position = { 0, 0 };
    
    if (params.fullscreen) {
        GLFWmonitor * const monitor = glfwGetPrimaryMonitor();
        
        if (monitor) {
            GLFWvidmode const * const mode = glfwGetVideoMode(monitor);
            
            struct window_size const monitor_display = (struct window_size) {
                .width = mode->width,
                .height = mode->height
            };

            window = glfwCreateWindow(monitor_display.width,
                                      monitor_display.height,
                                      params.title,
                                      glfwGetPrimaryMonitor(),
                                      NULL);
            
            // prefer centered window if initially fullscreen;
            // otherwise let the OS determine window placement
            position.x = (monitor_display.width / 2) - (display.width / 2);
            position.y = (monitor_display.height / 2) - (display.height / 2);
        }
    } else {
        window = glfwCreateWindow(display.width,
                                  display.height,
                                  params.title,
                                  NULL,
                                  NULL);
    }
    
    if (window == NULL) {
        return false;
    }
    
    glfwSetFramebufferSizeCallback(window, window_callback_size);
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(params.swap_interval);
    
    // gl3w must be initialized *after* window creation
    if (gl3wInit()) {
        fprintf(stderr, "gl3w failed to initialize");
        
        return NULL;
    }
    
    struct window_context * const context =
        malloc(sizeof(struct window_context));
    
    context->window = window;
    context->display = display;
    context->preferred_position = position;
    
    return context;
}

void
window_terminate(struct window_context * const context)
{
    glfwSetFramebufferSizeCallback(context->window, NULL);
    glfwSetErrorCallback(NULL);
    
    glfwTerminate();
    
    free(context);
}

void
window_set_closed(struct window_context * const context, bool const closed)
{
    glfwSetWindowShouldClose(context->window, closed);
}

bool
window_is_closed(struct window_context * const context)
{
    if (context->window != NULL) {
        return glfwWindowShouldClose(context->window);
    }
    
    return true;
}

bool
window_is_fullscreen(struct window_context const * const context)
{
    if (context->window != NULL) {
        return glfwGetWindowMonitor(context->window) != NULL;
    }
    
    return false;
}

void
window_set_fullscreen(struct window_context * const context,
                      bool const fullscreen)
{
    if (fullscreen) {
        window_make_windowed(context->window,
                             context->preferred_position,
                             context->display);
    } else {
        window_make_fullscreen(context->window,
                               &context->preferred_position);
    }
}

void
window_present(struct window_context const * const context)
{
    glfwSwapBuffers(context->window);
    glfwPollEvents();
}

void
window_read(struct window_context * const context,
            struct key_state * const state)
{
    bool down_previously[TERM_KEY_MAX];
    
    for (int32_t input = TERM_KEY_FIRST; input < TERM_KEY_MAX; input++) {
        down_previously[input] = state->down[input];
    }
    
    GLFWwindow * const window = context->window;

    bool const is_modified =
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
    
    state->down[TERM_KEY_UP] =
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    
    state->down[TERM_KEY_DOWN] =
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    
    state->down[TERM_KEY_LEFT] =
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    
    state->down[TERM_KEY_RIGHT] =
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    
    state->down[TERM_KEY_CONFIRM] =
        (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) && !is_modified;
    
    state->down[TERM_KEY_ESCAPE] =
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !is_modified;
    
    state->down[TERM_KEY_SPACE] =
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !is_modified;
    
    state->down[TERM_KEY_ANY] = false;
    
    for (int32_t input = TERM_KEY_FIRST; input < TERM_KEY_MAX; input++) {
        if (state->down[input]) {
            state->down[TERM_KEY_ANY] = true;
        }
        
        state->released[input] = down_previously[input] && !state->down[input];
        state->pressed[input] = !down_previously[input] && state->down[input];
    }
}

static
void
window_make_windowed(GLFWwindow * const window,
                     struct window_position const position,
                     struct window_size const size)
{
    int32_t const refresh_rate = GLFW_DONT_CARE;
    
    glfwSetWindowMonitor(window, NULL,
                         position.x, position.y,
                         size.width, size.height,
                         refresh_rate);
}

static
void
window_make_fullscreen(GLFWwindow * const window,
                       struct window_position * const previous)
{
    GLFWmonitor * const monitor = glfwGetPrimaryMonitor();
    
    if (monitor) {
        GLFWvidmode const * const mode = glfwGetVideoMode(monitor);
        
        struct window_position const default_position = { 0, 0 };
        
        glfwGetWindowPos(window, &previous->x, &previous->y);
        glfwSetWindowMonitor(window, monitor,
                             default_position.x,
                             default_position.y,
                             mode->width, mode->height,
                             mode->refreshRate);
    }
}

static
void
window_callback_size(GLFWwindow * const window,
                     int32_t const width,
                     int32_t const height)
{
    (void)window;
    (void)width;
    (void)height;
    
    // todo: handle resizing; this will only happen when transitioning
    //       between windowed/fullscreen
}

static
void
window_callback_error(int32_t const error, char const * const description)
{
    fprintf(stderr, "GLFW: %d %s", error, description);
}
