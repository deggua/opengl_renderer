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

// TODO: I need to look more carefully at the depth test function used in each pass
// I think it might partially be responsible for some z-fighting I'm seeing

// TODO: need to look into early-z testing, not sure if I need an explicit depth pass or not, but apparently
// using discard (for the alpha test) effectively disables early-z

void RenderLoop(GLFWwindow* window)
{
    constexpr f32       fov       = 70.0f;
    constexpr glm::vec3 rgb_black = {0.0f, 0.0f, 0.0f};
    constexpr glm::vec3 rgb_white = {1.0f, 1.0f, 1.0f};

    Renderer renderer = Renderer(RENDER_ENABLE_OPENGL_LOGGING);
    renderer.Set_Resolution(g_res_w, g_res_h);
    renderer.Clear(rgb_black);

    std::vector<Object> objs = {
        Object("assets/sponza/sponza.obj").CastsShadows(true).Scale(0.01f),
        Object("assets/skull/12140_Skull_v3_L2.obj").CastsShadows(true).Scale(0.01f).Position({0, 3, 0}),
    };

    // TODO: how should we construct light sources?
    AmbientLight ambient_light = AmbientLight{rgb_white, 0.1f};

    SunLight sun_light = SunLight{
        {0.0f, -1.0f, 0.0f},
        rgb_white,
        1.0f
    };

    renderer.Enable(GL_DEPTH_TEST);
    renderer.Enable(GL_CULL_FACE);
    renderer.Enable(GL_BLEND);
    GL(glCullFace(GL_BACK));
    GL(glFrontFace(GL_CCW));
    GL(glBlendEquation(GL_FUNC_ADD));

    while (!glfwWindowShouldClose(window)) {
        UpdateTime(window);
        ProcessKeyboardInput(window);

        renderer.Clear(rgb_black);

        if (g_res_w != g_res_w_old || g_res_h != g_res_h_old) {
            renderer.Set_Resolution(g_res_w, g_res_h);
        }

        glm::mat4 mtx_screen = glm::perspective(glm::radians(fov), (f32)g_res_w / (f32)g_res_h, 0.1f, 100.0f);
        renderer.Set_ScreenMatrix(mtx_screen);

        // update camera
        glm::mat4 mtx_view = g_Camera.ViewMatrix();
        renderer.Set_ViewMatrix(mtx_view);
        renderer.Set_ViewPosition(g_Camera.pos);

        // TODO: we should iterate the light types in order to reduce the amount of program switching
        // not sure how that should be encapsulated however
        // TODO: it's also more efficient to render objects with the same texture/mesh together as well
        // not sure how that could be tracked either

        /* --- Pass :: Ambient Light + Depth --- */
        GL(glDisable(GL_STENCIL_TEST));

        GL(glBlendFunc(GL_ONE, GL_ZERO));
        GL(glDepthFunc(GL_LESS));

        for (const auto& obj : objs) {
            renderer.Render_Light(ambient_light, obj);
        }

        /* --- Pass :: Point Light :: Shadow Volume --- */
        GL(glEnable(GL_STENCIL_TEST));
        GL(glEnable(GL_DEPTH_CLAMP));
        GL(glDisable(GL_CULL_FACE));

        GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        GL(glDepthMask(GL_FALSE));

        GL(glStencilFunc(GL_ALWAYS, 0, 0xFF));
        GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP));
        GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP));

        GL(glDepthFunc(GL_LESS));

        for (const auto& obj : objs) {
            renderer.Render_Shadow(point_light, obj);
        }

        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glEnable(GL_CULL_FACE));

        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glDepthMask(GL_TRUE));

        /* --- Pass :: Point Light :: Direct Light --- */
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
        GL(glBlendFunc(GL_ONE, GL_ONE));

        GL(glDepthFunc(GL_LEQUAL));

        for (const auto& obj : objs) {
            renderer.Render_Light(point_light, obj);
        }

        // clear the stencil buffer for the second shadow pass
        GL(glClear(GL_STENCIL_BUFFER_BIT));

        /* --- Pass :: Sun Light :: Shadow Volume --- */
        GL(glEnable(GL_STENCIL_TEST));
        GL(glEnable(GL_DEPTH_CLAMP));
        GL(glDisable(GL_CULL_FACE));

        GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        GL(glDepthMask(GL_FALSE));

        GL(glStencilFunc(GL_ALWAYS, 0, 0xFF));
        GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP));
        GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP));

        GL(glDepthFunc(GL_LESS));

        for (const auto& obj : objs) {
            renderer.Render_Shadow(sun_light, obj);
        }

        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glEnable(GL_CULL_FACE));

        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glDepthMask(GL_TRUE));

        /* --- Pass :: Sun Light :: Direct Lighting --- */
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
        GL(glBlendFunc(GL_ONE, GL_ONE));

        GL(glDepthFunc(GL_LEQUAL));

        for (const auto& obj : objs) {
            renderer.Render_Light(sun_light, obj);
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
