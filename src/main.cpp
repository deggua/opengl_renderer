#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"
#include "common/opengl.hpp"

using namespace glm;

struct Vertex {
    vec3 pos;
    vec3 rgb;
    vec2 tex;
};

static const Vertex vertices[] = {
    {  .pos = {0.5f, 0.5f, 0.0f}, .rgb = {1.0f, 0.0f, 0.0f}, .tex = {1.0f, 1.0f}},
    { .pos = {0.5f, -0.5f, 0.0f}, .rgb = {0.0f, 1.0f, 0.0f}, .tex = {1.0f, 0.0f}},
    { .pos = {-0.5f, 0.5f, 0.0f}, .rgb = {1.0f, 1.0f, 1.0f}, .tex = {0.0f, 1.0f}},
    {.pos = {-0.5f, -0.5f, 0.0f}, .rgb = {0.0f, 0.0f, 1.0f}, .tex = {0.0f, 0.0f}},
};

static const GLuint indices[] = {0, 1, 2, 1, 2, 3};

f32 BlendValue = 0.2f;

SHADER_FILE(VertexShader);
SHADER_FILE(FragmentShader);

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
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        BlendValue = BlendValue < 1.0f ? BlendValue + 0.01f : BlendValue;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        BlendValue = BlendValue > 0.0f ? BlendValue - 0.01f : BlendValue;
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

template<typename T>
static void SetUniform(GLuint program, const char* name, T value)
{
    GLint location;
    GL(location = glGetUniformLocation(program, name));

    if constexpr (std::is_same_v<T, bool>) {
        GL(glUniform1i(location, (GLint)value));
    } else if constexpr (std::is_same_v<T, GLint>) {
        GL(glUniform1i(location, value));
    } else if constexpr (std::is_same_v<T, GLuint>) {
        GL(glUniform1ui(location, value));
    } else if constexpr (std::is_same_v<T, f32>) {
        GL(glUniform1f(location, value));
    } else if constexpr (std::is_same_v<T, vec2>) {
        GL(glUniform2f(location, value.x, value.y));
    } else if constexpr (std::is_same_v<T, vec3>) {
        GL(glUniform3f(location, value.x, value.y, value.z));
    } else if constexpr (std::is_same_v<T, vec4>) {
        GL(glUniform4f(location, value.x, value.y, value.z, value.w));
    } else if constexpr (std::is_same_v<T, mat4>) {
        GL(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value)));
    } else {
        // TODO: is there a better way to do this?
        static_assert(!std::is_same_v<T, T>, "No associated OpenGL function for templated type T");
    }
}

static GLuint LoadTexture(const char* file_path)
{
    GLuint texture;

    GL(glGenTextures(1, &texture));
    GL(glBindTexture(GL_TEXTURE_2D, texture));

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    int width, height, num_channels;
    u8* image_data = stbi_load(file_path, &width, &height, &num_channels, 0);

    if (!image_data) {
        ABORT("Failed to load texture: %s", file_path);
    }

    GLenum color_fmt = (num_channels == 3 ? GL_RGB : GL_RGBA);

    GL(glTexImage2D(GL_TEXTURE_2D, 0, color_fmt, width, height, 0, color_fmt, GL_UNSIGNED_BYTE, image_data));
    GL(glGenerateMipmap(GL_TEXTURE_2D));

    stbi_image_free(image_data);

    return texture;
}

int main()
{
    stbi_set_flip_vertically_on_load(true);

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
    GLuint vertex_shader   = CompileShader(GL_VERTEX_SHADER, VertexShader.src, VertexShader.len);
    GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, FragmentShader.src, FragmentShader.len);

    GLuint shader_program = LinkShaders(vertex_shader, fragment_shader);

    // setup VBO, VAO, EBO
    GLuint vao;
    GL(glGenVertexArrays(1, &vao));

    GLuint vbo;
    GL(glGenBuffers(1, &vbo));

    GLuint ebo;
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
    GL(glVertexAttribOffset(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, pos)));
    GL(glEnableVertexAttribArray(0));

    GL(glVertexAttribOffset(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, rgb)));
    GL(glEnableVertexAttribArray(1));

    GL(glVertexAttribOffset(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex)));
    GL(glEnableVertexAttribArray(2));

    // wireframe
    // GL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    GLuint tex_wall = LoadTexture("assets/wall.jpg");
    GLuint tex_face = LoadTexture("assets/face.png");

    GL(glUseProgram(shader_program));
    SetUniform(shader_program, "texture_0", 0);
    SetUniform(shader_program, "texture_1", 1);

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // handle user input
        ProcessInput(window);

        // clear the background
        GL(glClear(GL_COLOR_BUFFER_BIT));

        // draw

        // set uniform based on time
        f32 time = glfwGetTime();

        mat4 transform = mat4(1.0f);
        transform      = rotate(transform, radians(time), vec3(0.0f, 0.0f, 1.0f));
        transform      = scale(transform, vec3(0.5f, 0.5f, 0.5f));
        SetUniform(shader_program, "transform", transform);

        SetUniform(shader_program, "blend_amt", BlendValue);

        GL(glActiveTexture(GL_TEXTURE0));
        GL(glBindTexture(GL_TEXTURE_2D, tex_wall));

        GL(glActiveTexture(GL_TEXTURE1));
        GL(glBindTexture(GL_TEXTURE_2D, tex_face));

        GL(glUseProgram(shader_program));
        GL(glBindVertexArray(vao));
        GL(glDrawElements(GL_TRIANGLES, lengthof(indices), GL_UNSIGNED_INT, 0));
        // GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(vertices)));

        // swap buffers, update window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
