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

PointLight point_light = PointLight{
    {0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
    10.0f
};

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

    point_light.pos = g_Camera.pos + 2.0f * dir_forward;
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

void RenderInit(GLFWwindow* window)
{
    (void)window;

    TexturePool.Load(DefaultTexture_Diffuse, true, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    TexturePool.Load(DefaultTexture_Specular, true, glm::vec4(0.0f));
}

// TODO: need to look into early-z testing, not sure if I need an explicit depth pass or not, but apparently
// using discard (for the alpha test) effectively disables early-z

void RenderLoop(GLFWwindow* window)
{
    constexpr glm::vec3 rgb_white = {1.0f, 1.0f, 1.0f};

    Renderer rt = Renderer(RENDER_ENABLE_OPENGL_LOGGING).ClearColor(0, 0, 0);

    std::vector<Object> objs = {
        Object("assets/sponza/sponza.obj").CastsShadows(true).Scale(0.01f),
        // Object("assets/sponza2/sponza.obj").CastsShadows(true),
        // Object("assets/sanmiguel/san-miguel-low-poly.obj").CastsShadows(true),
        // Object("assets/sibenik/sibenik.obj").CastsShadows(true),
        // Object("assets/breakfast/breakfast_room.obj").CastsShadows(true),
        // Object("assets/vokselia/vokselia_spawn.obj").CastsShadows(true).Scale(10.0f),

        // Object("assets/default.obj").CastsShadows(true),
    };

    // TODO: how should we construct light sources?
    AmbientLight ambient_light = AmbientLight{
        rgb_white,
        0.2f,
    };

    SunLight sun_light = SunLight{
        glm::normalize(glm::vec3{-1.0f, -1.0f, 0.0f}),
        rgb_white,
        1.0f,
    };

    while (!glfwWindowShouldClose(window)) {
        UpdateTime(window);
        ProcessKeyboardInput(window);

        // setup rendering parameters
        rt.Resolution(g_res_w, g_res_h);
        rt.ViewPosition(g_Camera.pos);
        rt.ViewMatrix(g_Camera.ViewMatrix());

        // TODO: it's also more efficient to render objects with the same texture/mesh together as well
        // not sure how that could be tracked

        rt.RenderPrepass();
        rt.RenderLighting(ambient_light, objs);
        rt.RenderLighting(sun_light, objs);
        rt.RenderLighting(point_light, objs);

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
