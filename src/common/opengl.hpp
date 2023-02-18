#pragma once

#define GLFW_INCLUDE_NONE

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#if RENDER_CHECK_OPENGL_CALLS
static const char* glGetErrorString(GLenum err)
{
    switch (err) {
        case GL_NO_ERROR:
            return "GL_NO_ERROR";

        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";

        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";

        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";

        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";

        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";

        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";

        case GL_TABLE_TOO_LARGE:
            return "GL_TABLE_TOO_LARGE";

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return "GL_INVALID_FRAMEBUFFER_OPERATION";

        case GL_CONTEXT_LOST:
            return "GL_CONTEXT_LOST";

        default:
            return "GL_???";
    }
}

#    define GL(...)                                                             \
        do {                                                                    \
            __VA_ARGS__;                                                        \
            GLenum err_;                                                        \
            if ((err_ = glGetError()) != GL_NO_ERROR) {                         \
                ABORT("OpenGL Error :: %s (%u)", glGetErrorString(err_), err_); \
            }                                                                   \
        } while (0)

#else
#    define GL(...) __VA_ARGS__
#endif
