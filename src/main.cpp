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
#include "math/random.hpp"

SunLight sun_light
    = SunLight().Direction({-1.0f, -1.0f, 0.0f}).Color(1.0f, 1.0f, 1.0f).Intensity(1.0f);
SpotLight spot_light = SpotLight()
                           .Direction({0.0f, -1.0f, 0.0f})
                           .Position(0.0f, 0.0f, 0.0f)
                           .Color(1.0f, 1.0f, 1.0f)
                           .Intensity(10.0f)
                           .Cutoff(30.0f, 45.0f);

std::vector<PointLight> point_lights = {};
std::vector<Sprite3D>   sprites      = {};

PlayerCamera g_Camera = PlayerCamera{
    .pos = glm::vec3(0.0f, 0.0f, 3.0f),
    .up  = glm::vec3(0.0f, 1.0f, 0.0f),
    .yaw = -90.0f,
};

f32 g_dt = 0.0f;
f32 g_t  = 0.0f;

u32 g_res_w = 1920;
u32 g_res_h = 1080;

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

    spot_light.Position(g_Camera.pos + 0.25f * dir_up).Direction(dir_forward);

    static bool prev_r_key = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && prev_r_key == false) {
        point_lights.clear();
        sprites.clear();
        prev_r_key = true;
    } else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE && prev_r_key == true) {
        prev_r_key = false;
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
    g_Camera.pitch = glm::clamp(g_Camera.pitch + yoffset, -89.0f, 89.0f);
}

static void ProcessMouseButtonInput(GLFWwindow* window, int button, int action, int mods)
{
    (void)window;
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        sun_light.dir = g_Camera.FacingDirection();
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        constexpr glm::vec3 light_color     = {1.0f, 0.7f, 0.1f};
        constexpr f32       light_intensity = 10.0f;

        point_lights.push_back(PointLight(g_Camera.pos, light_color, light_intensity));
        sprites.push_back(Sprite3D("assets/flare.png")
                              .Position(g_Camera.pos)
                              .Tint(light_color)
                              .Scale(0.3f)
                              .Intensity(10.0f));
    }
}

static void ProcessMouseScrollInput(GLFWwindow* window, double xoffset, double yoffset)
{
    (void)window;
    (void)xoffset;

    f32 angle_adj_deg   = yoffset * 1.0f;
    f32 cur_angle_inner = spot_light.InnerCutoff();
    f32 cur_angle_outer = spot_light.OuterCutoff();
    f32 ratio           = cur_angle_outer / cur_angle_inner;

    f32 new_angle_inner = glm::clamp(cur_angle_inner + angle_adj_deg, 10.0f, 120.0f);
    f32 new_angle_outer = glm::clamp(new_angle_inner * ratio, 1.0f, 180.0f);

    spot_light.Cutoff(new_angle_inner, new_angle_outer);
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

// TODO: need to look into early-z testing, not sure if I need an explicit depth pass or not, but
// apparently using discard (for the alpha test) effectively disables early-z

void RenderLoop(GLFWwindow* window)
{
    constexpr glm::vec3 rgb_white = {1.0f, 1.0f, 1.0f};

    Renderer rt  = Renderer(RENDER_ENABLE_OPENGL_LOGGING).ClearColor(0, 0, 0).FOV(90);
    Skybox   sky = Skybox("assets/tex/sky0");

    std::vector<Object> objs = {
        Object("assets/sponza/sponza.obj").CastsShadows(true).Scale(0.01f),
        // Object("assets/sponza2/sponza.obj").CastsShadows(true),
        // Object("assets/sanmiguel/san-miguel-low-poly.obj").CastsShadows(true),
        // Object("assets/sibenik/sibenik.obj").CastsShadows(true),
        // Object("assets/breakfast/breakfast_room.obj").CastsShadows(true),
        // Object("assets/vokselia/vokselia_spawn.obj").CastsShadows(true).Scale(10.0f),

        // Object("assets/default.obj").CastsShadows(true),
    };

    AmbientLight ambient_light = AmbientLight(rgb_white, 0.05f);

    while (!glfwWindowShouldClose(window)) {
        PlayerCamera cam      = g_Camera;
        SunLight     sun_dupe = sun_light;
        SpotLight    sp_dupe  = spot_light;

        UpdateTime(window);
        ProcessKeyboardInput(window);

        // setup rendering parameters
        rt.Resolution(g_res_w, g_res_h);
        rt.ViewPosition(cam.pos);
        rt.ViewMatrix(cam.ViewMatrix());

        rt.RenderPrepass();
        {
            rt.RenderObjectLighting(ambient_light, objs);
            rt.RenderObjectLighting(sun_dupe, objs);

            for (const auto& light : point_lights) {
                rt.RenderObjectLighting(light, objs);
            }

            rt.RenderObjectLighting(sp_dupe, objs);
            rt.RenderSkybox(sky);

            rt.RenderSprite(sprites);
        }
        rt.RenderBloom();
        rt.RenderScreen();

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
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetCursorPosCallback(window, ProcessMouseInput);
    glfwSetMouseButtonCallback(window, ProcessMouseButtonInput);
    glfwSetScrollCallback(window, ProcessMouseScrollInput);

    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        ABORT("Failed to initialize GLEW\n");
    }

    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
    glfwSwapInterval(0);

    LOG_INFO("GLFW initialized, starting renderer...");

    try {
        RenderLoop(window);
    } catch (std::exception e) {
        ABORT("Exception thrown from Renderer: %s", e.what());
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}
