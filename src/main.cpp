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
#include "utils/profiling.hpp"

/// IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

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

f32 g_sharpening_strength = 1.0f;

u32 g_res_w_old = g_res_w;
u32 g_res_h_old = g_res_h;

bool g_ui_mode = false;

typedef void (*KeyCallback_OnTransition)(GLFWwindow* window, bool key_pressed);

static void Key_ESC_OnTransition(GLFWwindow* window, bool key_pressed)
{
    if (key_pressed) {
        glfwSetWindowShouldClose(window, true);
    }
}

static void Key_X_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_ui_mode = !g_ui_mode;
        SetProfilingEnabled(g_ui_mode);
    }
}

static void Key_T_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_sharpening_strength = g_sharpening_strength == 1.0f ? 0.0f : 1.0f;
    }
}

static void Key_R_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        point_lights.clear();
        sprites.clear();
    }
}

enum {
    MOVE_DIR_NONE     = 0,
    MOVE_DIR_UP       = (1 << 0),
    MOVE_DIR_DOWN     = (1 << 1),
    MOVE_DIR_LEFT     = (1 << 2),
    MOVE_DIR_RIGHT    = (1 << 3),
    MOVE_DIR_FORWARD  = (1 << 4),
    MOVE_DIR_BACKWARD = (1 << 5),
};

bool g_move_fast  = false;
int  g_move_flags = MOVE_DIR_NONE;

static void Key_W_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_FORWARD;
    } else {
        g_move_flags &= ~MOVE_DIR_FORWARD;
    }
}

static void Key_A_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_LEFT;
    } else {
        g_move_flags &= ~MOVE_DIR_LEFT;
    }
}

static void Key_S_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_BACKWARD;
    } else {
        g_move_flags &= ~MOVE_DIR_BACKWARD;
    }
}

static void Key_D_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_RIGHT;
    } else {
        g_move_flags &= ~MOVE_DIR_RIGHT;
    }
}

static void Key_SPACE_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_UP;
    } else {
        g_move_flags &= ~MOVE_DIR_UP;
    }
}

static void Key_CTRL_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_move_flags |= MOVE_DIR_DOWN;
    } else {
        g_move_flags &= ~MOVE_DIR_DOWN;
    }
}

static void Key_SHIFT_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    g_move_fast = key_pressed;
}

bool g_spotlight_on = true;

static void Key_F_OnTransition(GLFWwindow* window, bool key_pressed)
{
    (void)window;
    if (key_pressed) {
        g_spotlight_on = !g_spotlight_on;
    }
}

static void ProcessKeyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    (void)scancode;
    (void)action;
    (void)mods;

    static bool key_states[GLFW_KEY_LAST + 1] = {0};

    static constexpr KeyCallback_OnTransition on_transition[GLFW_KEY_LAST + 1] = {
        [GLFW_KEY_ESCAPE]        = Key_ESC_OnTransition,
        [GLFW_KEY_X]             = Key_X_OnTransition,
        [GLFW_KEY_T]             = Key_T_OnTransition,
        [GLFW_KEY_R]             = Key_R_OnTransition,
        [GLFW_KEY_W]             = Key_W_OnTransition,
        [GLFW_KEY_A]             = Key_A_OnTransition,
        [GLFW_KEY_S]             = Key_S_OnTransition,
        [GLFW_KEY_D]             = Key_D_OnTransition,
        [GLFW_KEY_SPACE]         = Key_SPACE_OnTransition,
        [GLFW_KEY_RIGHT_CONTROL] = Key_CTRL_OnTransition,
        [GLFW_KEY_LEFT_CONTROL]  = Key_CTRL_OnTransition,
        [GLFW_KEY_RIGHT_SHIFT]   = Key_SHIFT_OnTransition,
        [GLFW_KEY_LEFT_SHIFT]    = Key_SHIFT_OnTransition,
        [GLFW_KEY_F]             = Key_F_OnTransition,
    };

    bool key_pressed;
    if (action == GLFW_PRESS) {
        key_pressed = true;
    } else if (action == GLFW_RELEASE) {
        key_pressed = false;
    } else {
        return;
    }

    if (key_states[key] != key_pressed && on_transition[key] != NULL) {
        on_transition[key](window, key_pressed);
    }

    key_states[key] = key_pressed;
}

static void InputTick(void)
{
    float     move_speed = g_move_fast ? 3.0f * 2.5f : 2.5f;
    glm::vec3 move_dir   = {0, 0, 0};

    glm::vec3 dir_forward = g_Camera.FacingDirection();
    glm::vec3 dir_up      = g_Camera.UpDirection();
    glm::vec3 dir_right   = g_Camera.RightDirection();

    move_dir += (g_move_flags & MOVE_DIR_FORWARD) ? dir_forward : glm::vec3();
    move_dir -= (g_move_flags & MOVE_DIR_BACKWARD) ? dir_forward : glm::vec3();

    move_dir += (g_move_flags & MOVE_DIR_RIGHT) ? dir_right : glm::vec3();
    move_dir -= (g_move_flags & MOVE_DIR_LEFT) ? dir_right : glm::vec3();

    move_dir += (g_move_flags & MOVE_DIR_UP) ? dir_up : glm::vec3();
    move_dir -= (g_move_flags & MOVE_DIR_DOWN) ? dir_up : glm::vec3();

    if (length(move_dir) > 0.1f) {
        g_Camera.pos += normalize(move_dir) * move_speed * g_dt;
    }
    spot_light.Position(g_Camera.pos + 0.25f * dir_up).Direction(dir_forward);
}

static void ProcessMouseInput(GLFWwindow* window, double xpos_d, double ypos_d)
{
    (void)window;

    if (g_ui_mode) {
        return;
    }

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

    if (g_ui_mode) {
        return;
    }

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

    if (g_ui_mode) {
        return;
    }

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
    Renderer rt  = Renderer(RENDER_ENABLE_OPENGL_LOGGING).ClearColor(0, 0, 0).FOV(90);
    Skybox   sky = Skybox("assets/tex/sky0");

    std::vector<Object> objs = {
        // Object("assets/sponza/sponza.obj").CastsShadows(true).Scale(0.01f),
        // Object("assets/sponza2/sponza.obj").CastsShadows(true),
        // Object("assets/sanmiguel/san-miguel-low-poly.obj").CastsShadows(true),
        // Object("assets/sibenik/sibenik.obj").CastsShadows(true),
        // Object("assets/breakfast/breakfast_room.obj").CastsShadows(true),
        // Object("assets/vokselia/vokselia_spawn.obj").CastsShadows(true).Scale(10.0f),

        Object("assets/test.obj").CastsShadows(true).Scale(1.0f),
        // Object("assets/default.obj").CastsShadows(true),
    };

    AmbientLight ambient_light = AmbientLight({1.0f, 1.0f, 1.0f}, 0.05f);

    while (!glfwWindowShouldClose(window)) {
        PlayerCamera cam      = g_Camera;
        SunLight     sun_dupe = sun_light;
        SpotLight    sp_dupe  = spot_light;

        // Capture input
        glfwPollEvents();

        // Start IMGUI frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (g_ui_mode && ProfilingMeasurements.size()) {
            auto cpu_sorted = std::vector<std::pair<ProfilerScope, ProfilerMeasurement>>();
            auto gpu_sorted = std::vector<std::pair<ProfilerScope, ProfilerMeasurement>>();
            cpu_sorted.reserve(ProfilingMeasurements.size());
            gpu_sorted.reserve(ProfilingMeasurements.size());

            for (const auto& [scope, measurement] : ProfilingMeasurements) {
                cpu_sorted.push_back({scope, measurement});
                gpu_sorted.push_back({scope, measurement});
            }

            std::sort(cpu_sorted.begin(), cpu_sorted.end(), [](auto left, auto right) {
                return left.second.cpu_time > right.second.cpu_time;
            });

            std::sort(gpu_sorted.begin(), gpu_sorted.end(), [](auto left, auto right) {
                return left.second.gpu_time > right.second.gpu_time;
            });

            ImGui::Begin("Profiling Data", &g_ui_mode);

            ImGui::SliderFloat(
                "Scattering Coefficient",
                &VolumetricLighting_ScatteringCoefficient,
                0.0f,
                1.0f);
            ImGui::SliderFloat("Density", &VolumetricLighting_Density, 0.0f, 1.0f);
            ImGui::SliderFloat("Asymmetry", &VolumetricLigthing_Asymmetry, -1.0f, 1.0f);

            ImGui::Text("CPU");
            ImGui::Text("%-120s | %-15s | %-10s | %-s", "Function", "Tag", "Usage", "Hit Count");
            ImGui::Text(
                "----------------------------------------------------------------------------------"
                "---------------------------------------------------------------------------------"
                "-");
            const auto& cpu_total_scope  = cpu_sorted[0].first;
            uint64_t    cpu_total_cycles = cpu_sorted[0].second.cpu_time;
            ImGui::Text(
                "%-120s | %-15s | %9.04f%% | %" PRIu64,
                cpu_total_scope.function,
                cpu_total_scope.tag,
                100.0f,
                cpu_sorted[0].second.hit_count);

            for (size_t ii = 1; ii < cpu_sorted.size(); ii++) {
                const auto& entry_scope  = cpu_sorted[ii].first;
                const auto& entry_cycles = cpu_sorted[ii].second;

                ImGui::Text(
                    "%-120s | %-15s | %9.04f%% | %" PRIu64,
                    entry_scope.function,
                    entry_scope.tag,
                    100.0f * (float)entry_cycles.cpu_time / (float)cpu_total_cycles,
                    entry_cycles.hit_count);
            }

            ImGui::Text("");
            ImGui::Text("");
            ImGui::Text("GPU");
            ImGui::Text("%-120s | %-15s | %-10s | %-s", "Function", "Tag", "Usage", "Hit Count");
            ImGui::Text(
                "----------------------------------------------------------------------------------"
                "---------------------------------------------------------------------------------"
                "-");
            const auto& gpu_total_scope  = gpu_sorted[0].first;
            uint64_t    gpu_total_cycles = gpu_sorted[0].second.gpu_time;
            ImGui::Text(
                "%-120s | %-15s | %9.04f%% | %" PRIu64,
                gpu_total_scope.function,
                gpu_total_scope.tag,
                100.0f,
                gpu_sorted[0].second.hit_count);

            for (size_t ii = 1; ii < gpu_sorted.size(); ii++) {
                const auto& entry_scope  = gpu_sorted[ii].first;
                const auto& entry_cycles = gpu_sorted[ii].second;

                ImGui::Text(
                    "%-120s | %-15s | %9.04f%% | %" PRIu64,
                    entry_scope.function,
                    entry_scope.tag,
                    100.0f * (float)entry_cycles.gpu_time / (float)gpu_total_cycles,
                    entry_cycles.hit_count);
            }

            ImGui::End();
        }

        static size_t frame_counter = 0;
        if ((frame_counter++ % 1024) == 0) {
            ResetProfilingData();
        }

        UpdateTime(window);
        InputTick();

        if (g_ui_mode) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (glfwRawMouseMotionSupported()) {
                glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
            }
        }

        // setup rendering parameters
        ImGui::Render();

        PROFILE_BLOCK ("Render Pass") {
            rt.Resolution(g_res_w, g_res_h);
            rt.ViewPosition(cam.pos);
            rt.ViewMatrix(cam.ViewMatrix());

            rt.StartRender();
            {
                rt.RenderDepth(objs);
                rt.RenderObjectLighting(ambient_light, objs);
                rt.RenderObjectLighting(sun_dupe, objs);

                for (const auto& light : point_lights) {
                    rt.RenderObjectLighting(light, objs);
                }

                if (g_spotlight_on) {
                    rt.RenderObjectLighting(sp_dupe, objs);
                }
                rt.RenderSkybox(sky);

                rt.RenderSprites(sprites);

                rt.RenderVolumetricFog(ambient_light, sun_dupe);
            }
            rt.FinishGeometry();

            rt.RenderBloom(0.005f, 0.04f);
            rt.RenderTonemap(Renderer_PostFX::TONEMAP_ACES_APPROX);
            if (g_sharpening_strength != 0.0f) {
                rt.RenderSharpening(g_sharpening_strength);
            }

            rt.FinishRender(2.2f);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        };

        CollectProfilingData();
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
    glfwSetKeyCallback(window, ProcessKeyboardInput);

    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        ABORT("Failed to initialize GLEW\n");
    }

    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
    glfwSwapInterval(0);

    LOG_INFO("GLFW initialized");

    // Setup IMGUI context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup IMGUI style
    ImGui::StyleColorsDark();

    // Setup IMGUI Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    try {
        RenderLoop(window);
    } catch (std::exception e) {
        ABORT("Exception thrown from Renderer: %s", e.what());
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
