#include <termlike/input.h> // term_cursor_state, TERM_KEY_*

#include <stdio.h> // fprintf
#include <stdlib.h> // malloc, free, NULL
#include <stdint.h> // int32_t
#include <stdbool.h> // bool
#include <math.h> // floor

#if defined(__clang__)
 #pragma clang diagnostic push
 #pragma clang diagnostic ignored "-Wdocumentation"
#endif

// use gl3w to load opengl headers
#include <gl3w/GL/gl3w.h> // gl3w*
// which means GLFW should not do that
#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h> // glfw*, GLFW*

#if defined(__clang__)
 #pragma clang diagnostic pop
#endif

#include "../../keys.h" // term_key_state
#include "../window.h" // window_*

struct window_position {
    int32_t x;
    int32_t y;
};

struct window_context {
    /** A reference to the current window */
    GLFWwindow * window;
    /** Actual display size; e.g. operating resolution ⨉ pixel size */
    struct window_size display;
    /** Position saved for restoring window location */
    struct window_position stored_position;
};

static void window_callback_error(int32_t error, char const * description);
static void window_callback_scroll(GLFWwindow *, double x, double y);

static GLFWwindow * window_open(char const * title,
                                struct window_size display,
                                bool fullscreen,
                                struct window_position *);

static void window_make_windowed(GLFWwindow *,
                                 struct window_position,
                                 struct window_size);
static void window_make_fullscreen(GLFWwindow *,
                                   struct window_position * previous);

static void window_read_keys(GLFWwindow *, struct term_key_state *);
static void window_read_cursor(GLFWwindow *, struct term_cursor_state *);

struct cursor_scroll {
    double x_offset;
    double y_offset;
};

// statically maintain an accumulated set of cursor scroll offsets, assuming
// there's only ever going to be one mouse input device, and that cursor input
// is not tied to a single window context, but rather the system as a whole
static struct cursor_scroll accumulated_cursor_scroll = { 0, 0 };

struct window_context *
window_create(struct window_params const params)
{
    glfwSetErrorCallback(window_callback_error);
    
    if (glfwInit() != GLFW_TRUE) {
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
    
    struct window_position position;
    
    GLFWwindow * const window = window_open(params.title,
                                            params.display,
                                            params.fullscreen,
                                            &position);
    
    if (window == NULL) {
        window_terminate(NULL);
        
        return NULL;
    }
    
    glfwSetScrollCallback(window, window_callback_scroll);
    // setting swap interval requires an initialized window/OpenGL context
    glfwSwapInterval(params.swap_interval);
    
    if (gl3wInit() != GL3W_OK) {
        // initializing gl3w requires an OpenGL context
        fprintf(stderr, "gl3w failed to initialize");
        
        window_terminate(NULL);
        
        return NULL;
    }
    
    struct window_context * const context =
        malloc(sizeof(struct window_context));
    
    context->window = window;
    context->display = params.display;
    context->stored_position = position;
    
    return context;
}

void
window_terminate(struct window_context * const context)
{
    glfwSetErrorCallback(NULL);
    
    if (context != NULL) {
        glfwSetScrollCallback(context->window, NULL);
    }
    
    glfwTerminate();
    
    if (context != NULL) {
        free(context);
    }
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
        window_make_fullscreen(context->window,
                               &context->stored_position);
    } else {
        window_make_windowed(context->window,
                             context->stored_position,
                             context->display);
    }
}

void
window_present(struct window_context const * const context)
{
    glfwSwapBuffers(context->window);
    glfwPollEvents();
}

void
window_get_framebuffer_size(struct window_context const * const context,
                            int32_t * width,
                            int32_t * height)
{
    // determine pixel size of the framebuffer for the window
    // this size is not necesarilly equal to the size of the window, as some
    // platforms may increase the pixel count (e.g. doubling on retina screens)
    glfwGetFramebufferSize(context->window, width, height);
}

void
window_read(struct window_context * const context,
            struct term_key_state * const keys,
            struct term_cursor_state * const cursor)
{
    GLFWwindow * const window = context->window;
    
    window_read_keys(window, keys);
    window_read_cursor(window, cursor);
}

static
void
window_read_keys(GLFWwindow * const window,
                 struct term_key_state * const keys)
{
    bool down_previously[TERM_KEY_MAX];
    
    for (int32_t input = TERM_KEY_FIRST; input < TERM_KEY_MAX; input++) {
        down_previously[input] = keys->down[input];
    }
    
    bool const is_modified =
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
    
    keys->down[TERM_KEY_UP] =
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    
    keys->down[TERM_KEY_DOWN] =
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    
    keys->down[TERM_KEY_LEFT] =
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    
    keys->down[TERM_KEY_RIGHT] =
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    
    keys->down[TERM_KEY_CONFIRM] =
        (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
         glfwGetKey(window, GLFW_KEY_KP_ENTER) == GLFW_PRESS) && !is_modified;
    
    keys->down[TERM_KEY_ESCAPE] =
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && !is_modified;
    
    keys->down[TERM_KEY_SPACE] =
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !is_modified;
    
    keys->down[TERM_KEY_TOGGLE_FULLSCREEN] =
        glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS &&
        glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
    
    keys->down[TERM_KEY_MOUSE_LEFT] =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    keys->down[TERM_KEY_MOUSE_RIGHT] =
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    
    keys->down[TERM_KEY_ANY] = false;
    
    for (int32_t input = TERM_KEY_FIRST; input < TERM_KEY_MAX; input++) {
        if (keys->down[input]) {
            keys->down[TERM_KEY_ANY] = true;
        }
        
        keys->released[input] = down_previously[input] && !keys->down[input];
        keys->pressed[input] = !down_previously[input] && keys->down[input];
    }
}

static
void
window_read_cursor(GLFWwindow * const window,
                   struct term_cursor_state * const cursor)
{
    double cursor_x = 0;
    double cursor_y = 0;
    
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    
    cursor->location.x = (int32_t)floor(cursor_x);
    cursor->location.y = (int32_t)floor(cursor_y);
    
    cursor->scroll.horizontal = accumulated_cursor_scroll.x_offset;
    cursor->scroll.vertical = accumulated_cursor_scroll.y_offset;
}

static
GLFWwindow *
window_open(char const * const title,
            struct window_size const display,
            bool const fullscreen,
            struct window_position * const position)
{
    GLFWwindow * window = NULL;
    
    // position is used for restoring window location when switching between
    // fullscreen and windowed; it is not used to initially place the window
    position->x = 0;
    position->y = 0;
    
    if (fullscreen) {
        GLFWmonitor * const monitor = glfwGetPrimaryMonitor();
        
        if (monitor) {
            GLFWvidmode const * const display_mode = glfwGetVideoMode(monitor);
            
            // make windows appear as centered when switching from fullscreen
            // as they would otherwise appear at (0, 0)- corner of the screen
            position->x = (display_mode->width / 2) - (display.width / 2);
            position->y = (display_mode->height / 2) - (display.height / 2);
            
            window = glfwCreateWindow(display_mode->width,
                                      display_mode->height,
                                      title,
                                      monitor,
                                      NULL);
        }
    } else {
        window = glfwCreateWindow(display.width,
                                  display.height,
                                  title,
                                  NULL,
                                  NULL);
    }
    
    if (window != NULL) {
        glfwMakeContextCurrent(window);
    }
    
    return window;
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
window_callback_scroll(GLFWwindow * const window,
                       double const x,
                       double const y)
{
    (void)window;
    
    accumulated_cursor_scroll.x_offset += x;
    accumulated_cursor_scroll.y_offset += y;
}

static
void
window_callback_error(int32_t const error, char const * const description)
{
    fprintf(stderr, "GLFW: %d %s", error, description);
}
