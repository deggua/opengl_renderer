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
    vec2 tex;
};

static const Vertex vertices[] = {
    {.pos = {0.0f, 0.0f, 0.0f}, .tex = {0.0f, 0.0f}},
    {.pos = {1.0f, 0.0f, 0.0f}, .tex = {1.0f, 0.0f}},
    {.pos = {0.0f, 1.0f, 0.0f}, .tex = {0.0f, 1.0f}},
    {.pos = {1.0f, 1.0f, 0.0f}, .tex = {1.0f, 1.0f}},
    {.pos = {0.0f, 0.0f, 1.0f}, .tex = {0.0f, 0.0f}},
    {.pos = {1.0f, 0.0f, 1.0f}, .tex = {1.0f, 0.0f}},
    {.pos = {0.0f, 1.0f, 1.0f}, .tex = {0.0f, 1.0f}},
    {.pos = {1.0f, 1.0f, 1.0f}, .tex = {1.0f, 1.0f}},
};

struct Triangle {
    GLuint idx[3];
};

static const Triangle indices[] = {
    {.idx = {1, 0, 3}}, // back
    {.idx = {0, 2, 3}}, // back
    {.idx = {4, 5, 6}}, // front
    {.idx = {5, 7, 6}}, // front
    {.idx = {6, 7, 3}}, // top
    {.idx = {6, 3, 2}}, // top
    {.idx = {0, 5, 4}}, // bottom
    {.idx = {0, 1, 5}}, // bottom
    {.idx = {0, 6, 2}}, // left
    {.idx = {0, 4, 6}}, // left
    {.idx = {5, 3, 7}}, // right
    {.idx = {5, 1, 3}}, // right
};

static const vec3 positions[] = {
    { 0.0f,  0.0f,   0.0f},
    { 2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f,  -2.5f},
    {-3.8f, -2.0f, -12.3f},
    { 2.4f, -0.4f,  -3.5f},
    {-1.7f,  3.0f,  -7.5f},
    { 1.3f, -2.0f,  -2.5f},
    { 1.5f,  2.0f,  -2.5f},
    { 1.5f,  0.2f,  -1.5f},
    {-1.3f,  1.0f,  -1.5f},
};

struct TargetCamera {
    vec3 pos;
    vec3 target;
    vec3 up;

    mat4 ViewMatrix()
    {
        return lookAt(this->pos, this->target, this->up);
    }
};

struct PlayerCamera {
    vec3 pos;
    vec3 up;

    f32 pitch;
    f32 yaw;
    f32 roll;

    vec3 FacingDirection()
    {
        vec3 direction = {
            cos(radians(yaw)) * cos(radians(pitch)),
            sin(radians(pitch)),
            sin(radians(yaw)) * cos(radians(pitch)),
        };
        return normalize(direction);
    }

    mat4 ViewMatrix()
    {
        return lookAt(this->pos, this->pos + this->FacingDirection(), this->up);
    }
};

PlayerCamera g_Camera = PlayerCamera{
    .pos = vec3(0.0f, 0.0f, 3.0f),
    .up  = vec3(0.0f, 1.0f, 0.0f),
    .yaw = -90.0f,
};

f32 g_dt = 0.0f;
f32 g_t  = 0.0f;

GLuint g_ShaderProgram = 0;

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
static void SetUniform(GLuint program, const char* name, const T& value)
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

static void ProcessKeyboardInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    constexpr float move_speed = 2.5f;

    vec3 dir_forward = g_Camera.FacingDirection();
    vec3 dir_up      = g_Camera.up;
    vec3 dir_right   = cross(dir_forward, dir_up);

    vec3 dir_move = {};

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        dir_move += dir_forward;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        dir_move -= dir_forward;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        dir_move -= dir_right;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        dir_move += dir_right;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        dir_move += dir_up;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
        dir_move -= dir_up;
    }

    if (length(dir_move) > 0.1f) {
        g_Camera.pos += normalize(dir_move) * move_speed * g_dt;
    }
}

static void ProcessMouseInput(GLFWwindow* window, double xpos_d, double ypos_d)
{
    float xpos = xpos_d;
    float ypos = ypos_d;

    static bool first_mouse = true;

    static float xpos_prev = 0.0f;
    static float ypos_prev = 0.0f;

    if (first_mouse) {
        xpos_prev   = xpos;
        ypos_prev   = ypos;
        first_mouse = false;
    }

    float xoffset = xpos - xpos_prev;
    float yoffset = -(ypos - ypos_prev);
    xpos_prev     = xpos;
    ypos_prev     = ypos;

    constexpr float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    g_Camera.yaw += xoffset;
    g_Camera.pitch = clamp(g_Camera.pitch + yoffset, -89.0f, 89.0f);
}

int main()
{
    constexpr u32 res_w        = 1280;
    constexpr u32 res_h        = 720;
    constexpr f32 aspect_ratio = (f32)res_w / (f32)res_h;
    constexpr f32 fov          = 70.0f;

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

    GLFWwindow* window = glfwCreateWindow(res_w, res_h, "Learn OpenGL", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        ABORT("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, ProcessMouseInput);

    // init glew
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        ABORT("Failed to initialize GLEW\n");
    }

    // setup viewport and clear color
    GL(glViewport(0, 0, res_w, res_h));
    GL(glClearColor(0.2f, 0.3f, 0.3f, 1.0f));

    // resize window callback
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);

    // setup shaders
    GLuint vertex_shader   = CompileShader(GL_VERTEX_SHADER, VertexShader.src, VertexShader.len);
    GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, FragmentShader.src, FragmentShader.len);

    g_ShaderProgram = LinkShaders(vertex_shader, fragment_shader);

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

    GL(glVertexAttribOffset(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex)));
    GL(glEnableVertexAttribArray(1));

    // wireframe
    // GL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    GLuint tex_wall = LoadTexture("assets/wall.jpg");
    GLuint tex_face = LoadTexture("assets/face.png");

    GL(glUseProgram(g_ShaderProgram));
    SetUniform(g_ShaderProgram, "texture_0", 0);
    SetUniform(g_ShaderProgram, "texture_1", 1);

    mat4 proj = perspective(radians(fov), aspect_ratio, 0.1f, 100.0f);
    SetUniform(g_ShaderProgram, "mat_proj", proj);

    GL(glEnable(GL_DEPTH_TEST));

    // render loop
    while (!glfwWindowShouldClose(window)) {
        // handle user input
        ProcessKeyboardInput(window);

        // clear the background
        GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

        // draw

        // set uniform based on time

        GL(glActiveTexture(GL_TEXTURE0));
        GL(glBindTexture(GL_TEXTURE_2D, tex_wall));

        GL(glActiveTexture(GL_TEXTURE1));
        GL(glBindTexture(GL_TEXTURE_2D, tex_face));

        GL(glUseProgram(g_ShaderProgram));
        GL(glBindVertexArray(vao));

        // update time
        f32 time = glfwGetTime();
        g_dt     = time - g_t;
        g_t      = time;

        SetUniform(g_ShaderProgram, "mat_view", g_Camera.ViewMatrix());

        for (size_t ii = 0; ii < lengthof(positions); ii++) {
            mat4 model = mat4(1.0f);
            model      = translate(model, positions[ii]);

            SetUniform(g_ShaderProgram, "mat_model", model);

            GL(glDrawElements(GL_TRIANGLES, lengthof(indices) * 3, GL_UNSIGNED_INT, 0));
        }

        // swap buffers, update window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
