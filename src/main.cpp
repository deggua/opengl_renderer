#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "common.hpp"
#include "common/opengl.hpp"

using namespace glm;

static const vec3 vertices[] = {
    vec3(0.5f, 0.5f, 0.0f),
    vec3(0.5f, -0.5f, 0.0f),
    vec3(-0.5f, -0.5f, 0.0f),
    vec3(-0.5f, 0.5f, 0.0f),
};

static const GLuint indices[] = {0, 1, 3, 1, 2, 3};

extern "C" const char     VertexShader[];
extern "C" const uint32_t VertexShader_size;

extern "C" const char     FragmentShader[];
extern "C" const uint32_t FragmentShader_size;

static void ErrorHandlerCallback(int error_code, const char* description)
{
    ABORT("GLFW Error :: %s (%d)", description, error_code);
}

static void WindowResizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    GL(glViewport(0, 0, width, height));
}

static void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

static GLuint CompileShader(GLenum shader_type, const char* src, u32 len)
{
    ASSERT(len < (u32)INT32_MAX);
    i32 i_len = len;

    GLuint shader;
    GL(shader = glCreateShader(shader_type));
    GL(glShaderSource(shader, 1, &src, &i_len));
    GL(glCompileShader(shader));

    // check vertex shader compile status
    if constexpr (RENDER_CHECK_SHADER_COMPILE) {
        GLint success;
        char  info_log[RENDER_SHADER_LOG_SIZE];
        GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));

        if (!success) {
            GL(glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log));
            ABORT(
                "Compilation of shader failed:\n"
                "Reason: %s\n",
                info_log);
        }
    }

    return shader;
}

template<class T, class... Ts>
static void AttachShaders(GLuint program, T first_shader, Ts... shaders)
{
    GL(glAttachShader(program, first_shader));
    if constexpr (sizeof...(shaders) > 0) {
        AttachShaders(program, shaders...);
    }
}

template<class... Ts>
static GLuint LinkShaders(Ts... shaders)
{
    // create program
    GLuint shader_program;
    GL(shader_program = glCreateProgram());

    // attach shaders to the program
    AttachShaders(shader_program, shaders...);

    // link shaders
    GL(glLinkProgram(shader_program));

    // check linker result
    if constexpr (RENDER_CHECK_SHADER_COMPILE) {
        GLint success;
        char  info_log[RENDER_SHADER_LOG_SIZE];
        GL(glGetProgramiv(shader_program, GL_LINK_STATUS, &success));

        if (!success) {
            GL(glGetProgramInfoLog(shader_program, sizeof(info_log), nullptr, info_log));
            ABORT(
                "Linking of shader program failed:\n"
                "Reason: %s\n",
                info_log);
        }
    }

    return shader_program;
}

int main()
{
    // glfw error callback
    glfwSetErrorCallback(ErrorHandlerCallback);

    // init glfw and setup window
    if (glfwInit() != GLFW_TRUE) {
        ABORT("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Learn OpenGL", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        ABORT("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    // init glew
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        ABORT("Failed to initialize GLEW\n");
    }

    // setup viewport and clear color
    GL(glViewport(0, 0, 1280, 720));
    GL(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));

    // resize window callback
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);

    // setup shaders
    GLuint vertex_shader   = CompileShader(GL_VERTEX_SHADER, VertexShader, VertexShader_size);
    GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, FragmentShader, FragmentShader_size);
    GLuint shader_program  = LinkShaders(vertex_shader, fragment_shader);

    // setup VBO, VAO, EBO
    GLuint vbo, vao, ebo;
    GL(glGenVertexArrays(1, &vao));
    GL(glGenBuffers(1, &vbo));
    GL(glGenBuffers(1, &ebo));

    // the following buffer binding will be attached to the VAO
    GL(glBindVertexArray(vao));

    // bind indices
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));

    // bind vertices
    GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    // set vertex attribute info
    GL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr));
    GL(glEnableVertexAttribArray(0));

    GL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // handle user input
        ProcessInput(window);

        // clear the background
        GL(glClear(GL_COLOR_BUFFER_BIT));

        // draw
        GL(glUseProgram(shader_program));
        GL(glBindVertexArray(vao));
        GL(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));

        // swap buffers, update window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
