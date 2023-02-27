#include <stdio.h>
#include <stdlib.h>

#include <string>

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

static const Vertex vertices[] = {
  // positions            // normals           // texture coords
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    { {0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {  {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {  {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    { {-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},

    { {-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    {  {0.5f, -0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {  {-0.5f, 0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    { {-0.5f, -0.5f, 0.5f},  {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

    {  {-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    { {-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    { {-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {  {-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

    {   {0.5f, 0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {  {0.5f, 0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    { {0.5f, -0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    { {0.5f, -0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {  {0.5f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {   {0.5f, 0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    { {0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {  {0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {  {0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    { {-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

    { {-0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {  {0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {  {-0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    { {-0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
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
    {0.0f, 0.0f, 0.0f},
};

PlayerCamera g_Camera = PlayerCamera{
    .pos = vec3(0.0f, 0.0f, 3.0f),
    .up  = vec3(0.0f, 1.0f, 0.0f),
    .yaw = -90.0f,
};

f32 g_dt = 0.0f;
f32 g_t  = 0.0f;

u32 g_res_w = 1280;
u32 g_res_h = 720;

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
    (void)window;

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

static void ErrorHandlerCallback(int error_code, const char* description)
{
    ABORT("GLFW Error :: %s (%d)", description, error_code);
}

static void WindowResizeCallback(GLFWwindow* window, int width, int height)
{
    (void)window;
    g_res_w = width;
    g_res_h = height;
}

int main()
{
    constexpr f32 fov = 70.0f;

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

    // resize window callback
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);

    // setup viewport and clear color
    GL(glViewport(0, 0, res_w, res_h));
    GL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

    // setup shaders
    GLuint vs_default = CompileShader(GL_VERTEX_SHADER, VertexShader.src, VertexShader.len);

    GLuint fs_ambient = CompileLightingShader(LightingType::AMBIENT, FragmentShader.src, FragmentShader.len);
    GLuint fs_point   = CompileLightingShader(LightingType::AMBIENT, FragmentShader.src, FragmentShader.len);
    GLuint fs_spot    = CompileLightingShader(LightingType::AMBIENT, FragmentShader.src, FragmentShader.len);
    GLuint fs_sun     = CompileLightingShader(LightingType::AMBIENT, FragmentShader.src, FragmentShader.len);

    GLuint sp_ambient = LinkShaders(vs_default, fs_ambient);
    GLuint sp_point   = LinkShaders(vs_default, fs_point);
    GLuint sp_spot    = LinkShaders(vs_default, fs_spot);
    GLuint sp_sun     = LinkShaders(vs_default, fs_sun);

    GLuint fs_light = CompileShader(GL_FRAGMENT_SHADER, FragmentShader_Light.src, FragmentShader_Light.len);
    GLuint sp_light = LinkShaders(vs_default, fs_light);

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
    GL(glVertexAttribOffset(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, norm)));
    GL(glEnableVertexAttribArray(1));
    GL(glVertexAttribOffset(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex)));
    GL(glEnableVertexAttribArray(2));

    GLuint light_vao;
    GL(glGenVertexArrays(1, &light_vao));
    GL(glBindVertexArray(light_vao));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
    GL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
    GL(glVertexAttribOffset(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, pos)));
    GL(glEnableVertexAttribArray(0));
    GL(glVertexAttribOffset(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, norm)));
    GL(glEnableVertexAttribArray(1));
    GL(glVertexAttribOffset(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tex)));
    GL(glEnableVertexAttribArray(2));

    // wireframe
    // GL(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));

    GLuint diffuse_map  = LoadTexture("assets/container.png");
    GLuint specular_map = LoadTexture("assets/container_specular.png");

    SetUniform(g_ShaderProgram, "material.diffuse", 0);
    SetUniform(g_ShaderProgram, "material.specular", 1);
    SetUniform(g_ShaderProgram, "material.shininess", 32.0f);

    SetUniform(g_ShaderProgram, "light.ambient", vec3(0.1f, 0.1f, 0.1f));
    SetUniform(g_ShaderProgram, "light.diffuse", vec3(1.0f, 1.0f, 1.0f));
    SetUniform(g_ShaderProgram, "light.specular", vec3(1.0f, 1.0f, 1.0f));

    mat4 mat_screen = perspective(radians(fov), aspect_ratio, 0.1f, 100.0f);
    SetUniform(sp_ambient, "mat_screen", mat_screen);
    SetUniform(sp_point, "mat_screen", mat_screen);
    SetUniform(sp_spot, "mat_screen", mat_screen);
    SetUniform(sp_sun, "mat_screen", mat_screen);
    SetUniform(sp_light, "mat_screen", mat_screen);

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
        GL(glBindTexture(GL_TEXTURE_2D, diffuse_map));
        GL(glActiveTexture(GL_TEXTURE1));
        GL(glBindTexture(GL_TEXTURE_2D, specular_map));

        GL(glUseProgram(g_ShaderProgram));
        GL(glBindVertexArray(vao));

        // update time
        f32 time = glfwGetTime();
        g_dt     = time - g_t;
        g_t      = time;

        // update camera
        mat4 mat_view = g_Camera.ViewMatrix();
        SetUniform(g_ShaderProgram, "mat_view", mat_view);
        SetUniform(g_ShaderProgram_Light, "mat_view", mat_view);

        // draw the light cube
        {
            mat4 mat_world = mat4(1.0f);
            vec3 light_pos = {0.0f, 2.0f, 5.0f};
            mat_world      = translate(mat_world, light_pos);
            SetUniform(g_ShaderProgram_Light, "mat_world", mat_world);

            GL(glBindVertexArray(light_vao));
            GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(vertices)));
            // GL(glDrawElements(GL_TRIANGLES, lengthof(indices) * 3, GL_UNSIGNED_INT, 0));
        }

        // draw normal objects

        GL(glUseProgram(g_ShaderProgram));
        for (size_t ii = 0; ii < lengthof(positions); ii++) {
            mat4 mat_world = mat4(1.0f);
            mat_world      = translate(mat_world, positions[ii]);
            SetUniform(g_ShaderProgram, "mat_world", mat_world);

            mat3 mat_normal = mat3(transpose(inverse(mat_world)));
            SetUniform(g_ShaderProgram, "mat_normal", mat_normal);

            GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(vertices)));
            // GL(glDrawElements(GL_TRIANGLES, lengthof(indices) * 3, GL_UNSIGNED_INT, 0));
        }

        // swap buffers, update window events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vao);
    glDeleteVertexArrays(1, &light_vao);
    glDeleteBuffers(1, &vbo);

    glfwTerminate();
    return EXIT_SUCCESS;
}
