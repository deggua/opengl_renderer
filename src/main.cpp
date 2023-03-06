#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"
#include "common/opengl.hpp"
#include "gfx/opengl.hpp"
#include "gfx/renderer.hpp"

static const std::vector<Vertex> vertices = {
  // positions            // normals           // texture coords
    { {0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {  {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    { {-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {  {0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
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
    { {0.5f, -0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {  {0.5f, 0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {  {0.5f, -0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    { {0.5f, -0.5f, -0.5f},  {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
    {   {0.5f, 0.5f, 0.5f},  {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},

    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    { {0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {  {0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {  {0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    { {-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},

    { {-0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {  {0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {  {-0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {   {0.5f, 0.5f, 0.5f},  {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    { {-0.5f, 0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
};

Light point_light = Light::Point({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, 2.0f);

PlayerCamera g_Camera = PlayerCamera{
    .pos = glm::vec3(0.0f, 0.0f, 3.0f),
    .up  = glm::vec3(0.0f, 1.0f, 0.0f),
    .yaw = -90.0f,
};

f32 g_dt = 0.0f;
f32 g_t  = 0.0f;

u32 g_res_w = 1280;
u32 g_res_h = 720;

u32 g_res_w_old = g_res_w;
u32 g_res_h_old = g_res_h;

static void ProcessKeyboardInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    float move_speed = 2.5f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        move_speed *= 3.0f;
    }

    glm::vec3 dir_forward = g_Camera.FacingDirection();
    glm::vec3 dir_up      = g_Camera.UpDirection();
    glm::vec3 dir_right   = g_Camera.RightDirection();

    glm::vec3 dir_move = {};

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

    point_light.point.pos = g_Camera.pos + 2.0f * dir_forward;
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
    g_Camera.pitch = glm::clamp(g_Camera.pitch + yoffset, -89.0f, 89.0f);
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

static void UpdateTime(GLFWwindow* window)
{
    f32 time = glfwGetTime();
    g_dt     = time - g_t;
    g_t      = time;

    char buf[32];
    snprintf(buf, sizeof(buf), "OpenGL | FPS = %d", (int)(1.0f / g_dt));
    glfwSetWindowTitle(window, buf);
}

void message_callback(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       length,
    const GLchar* message,
    const void*   user_param)
{
    const auto src_str = [source]() {
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "WINDOW SYSTEM";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "SHADER COMPILER";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "THIRD PARTY";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "APPLICATION";
            case GL_DEBUG_SOURCE_OTHER:
                return "OTHER";
        }
    }();

    const auto type_str = [type]() {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "ERROR";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "DEPRECATED_BEHAVIOR";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "UNDEFINED_BEHAVIOR";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "PORTABILITY";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "PERFORMANCE";
            case GL_DEBUG_TYPE_MARKER:
                return "MARKER";
            case GL_DEBUG_TYPE_OTHER:
                return "OTHER";
        }
    }();

    const auto severity_str = [severity]() {
        switch (severity) {
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "NOTIFICATION";
            case GL_DEBUG_SEVERITY_LOW:
                return "LOW";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "MEDIUM";
            case GL_DEBUG_SEVERITY_HIGH:
                return "HIGH";
        }
    }();
    std::cout << src_str << ", " << type_str << ", " << severity_str << ", " << id << ": " << message << '\n';
}

void RenderInit(GLFWwindow* window)
{
    (void)window;

    TexturePool.Insert("$NO_DIFFUSE", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    TexturePool.Insert("$NO_SPECULAR", glm::vec4(0.0f));

    // GL(glEnable(GL_DEBUG_OUTPUT));
    // GL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
    // GL(glDebugMessageCallback(message_callback, nullptr));
}

void RenderLoop(GLFWwindow* window)
{
    constexpr f32       fov       = 70.0f;
    constexpr glm::vec3 rgb_black = {0.0f, 0.0f, 0.0f};
    constexpr glm::vec3 rgb_white = {1.0f, 1.0f, 1.0f};

    Renderer renderer = Renderer();
    renderer.Set_Resolution(g_res_w, g_res_h);
    renderer.Clear(rgb_black);

    // std::vector<Object> objs = Object::LoadObjects(std::string("assets/skull/12140_Skull_v3_L2.obj"));
    std::vector<Object> objs = Object::LoadObjects("assets/sponza/sponza.obj");
    printf("Loaded %zu objects\n", objs.size());

    // TODO: how should we construct light sources?
    Light ambient_light = Light::Ambient(rgb_white, 0.1f);
    Light sun_light     = Light::Sun({0.0f, -1.0f, 0.0f}, rgb_white, 1.0f);

    Light* lights[] = {
        // sun_light,
        &point_light,
    };

    renderer.Enable(GL_DEPTH_TEST);

    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glFrontFace(GL_CCW));

    GL(glEnable(GL_BLEND));
    GL(glBlendEquation(GL_FUNC_ADD));

    while (!glfwWindowShouldClose(window)) {
        UpdateTime(window);
        ProcessKeyboardInput(window);

        renderer.Clear(rgb_black);

        // TODO: do this here? what about resolution? maybe we need a boolean to do this only if the resolution changed
        // update screen (if resolution changed)
        glm::mat4 mtx_screen = glm::perspective(glm::radians(fov), (f32)g_res_w / (f32)g_res_h, 0.1f, 100.0f);
        renderer.Set_ScreenMatrix(mtx_screen);
        if (g_res_w != g_res_w_old || g_res_h != g_res_h_old) {
            renderer.Set_Resolution(g_res_w, g_res_h);
        }

        // update camera
        glm::mat4 mtx_view = g_Camera.ViewMatrix();
        renderer.Set_ViewMatrix(mtx_view);
        renderer.Set_ViewPosition(g_Camera.pos);

        // TODO: we should iterate the light types in order to reduce the amount of program switching
        // not sure how that should be encapsulated however
        // TODO: it's also more efficient to render objects with the same texture/mesh together as well
        // not sure how that could be tracked either

        /* ------------------ */
        // ambient light pass
        GL(glDisable(GL_STENCIL_TEST));

        GL(glBlendFunc(GL_ONE, GL_ZERO));
        GL(glDepthFunc(GL_LESS));
        for (const auto& obj : objs) {
            glm::mat4 mtx_world = glm::mat4(1.0f);
            mtx_world           = glm::scale(mtx_world, glm::vec3(0.01f));
            // mtx_world           = glm::rotate(mtx_world, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            mtx_world = glm::translate(mtx_world, {0.0f, 0.0f, 0.0f});
            renderer.Set_WorldMatrix(mtx_world);

            glm::mat4 mtx_normal = glm::mat4(1.0f);
            mtx_normal           = glm::mat3(glm::transpose(glm::inverse(mtx_world)));
            renderer.Set_NormalMatrix(mtx_normal);

            renderer.Render(obj, ambient_light);
        }

        /* ------------------ */
        // shadow volume pass
        GL(glEnable(GL_STENCIL_TEST));
        GL(glEnable(GL_DEPTH_CLAMP));
        GL(glDisable(GL_CULL_FACE));

        GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        GL(glDepthMask(GL_FALSE));

        GL(glStencilFunc(GL_ALWAYS, 0, 0xFF));
        GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP));
        GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP));

        for (const auto& obj : objs) {
            glm::mat4 mtx_world = glm::mat4(1.0f);
            mtx_world           = glm::scale(mtx_world, glm::vec3(0.01f));
            // mtx_world           = glm::rotate(mtx_world, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            mtx_world = glm::translate(mtx_world, {0.0f, 0.0f, 0.0f});
            renderer.Set_WorldMatrix(mtx_world);

            glm::mat4 mtx_normal = glm::mat4(1.0f);
            mtx_normal           = glm::mat3(glm::transpose(glm::inverse(mtx_world)));
            renderer.Set_NormalMatrix(mtx_normal);

            renderer.ComputeShadows(obj, point_light);
        }

        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glEnable(GL_CULL_FACE));

        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glDepthMask(GL_TRUE));

        /* ------------------ */
        // direct lighting pass
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_KEEP));
        GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_KEEP));
        GL(glBlendFunc(GL_ONE, GL_ONE));
        GL(glDepthFunc(GL_LEQUAL));
        for (const auto& light : lights) {
            for (const auto& obj : objs) {
                glm::mat4 mtx_world = glm::mat4(1.0f);
                mtx_world           = glm::scale(mtx_world, glm::vec3(0.01f));
                // mtx_world           = glm::rotate(mtx_world, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                mtx_world = glm::translate(mtx_world, {0.0f, 0.0f, 0.0f});
                renderer.Set_WorldMatrix(mtx_world);

                glm::mat4 mtx_normal = glm::mat4(1.0f);
                mtx_normal           = glm::mat3(glm::transpose(glm::inverse(mtx_world)));
                renderer.Set_NormalMatrix(mtx_normal);

                renderer.Render(obj, *light);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main()
{
    stbi_set_flip_vertically_on_load(true);

    glfwSetErrorCallback(ErrorHandlerCallback);

    if (glfwInit() != GLFW_TRUE) {
        ABORT("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_res_w, g_res_h, "Learn OpenGL", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        ABORT("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, ProcessMouseInput);

    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        ABORT("Failed to initialize GLEW\n");
    }

    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
    glfwSwapInterval(0);

    RenderInit(window);
    RenderLoop(window);

    glfwTerminate();
    return EXIT_SUCCESS;
}
