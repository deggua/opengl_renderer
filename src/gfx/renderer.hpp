#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "common.hpp"
#include "gfx/assets.hpp"
#include "gfx/opengl.hpp"

struct AmbientLight {
    glm::vec3 color;

    f32 intensity;
};

struct SunLight {
    glm::vec3 dir;
    glm::vec3 color;

    f32 intensity;
};

struct SpotLight {
    glm::vec3 pos;
    glm::vec3 dir; // must be pre-normalized
    glm::vec3 color;

    f32 inner_cutoff; // dot(dir, inner_dir)
    f32 outer_cutoff; // dot(dit, outer_dir)
    f32 intensity;
};

struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;

    f32 intensity;
};

enum class LightType {
    Ambient,
    Point,
    Spot,
    Sun
};

struct TargetCamera {
    glm::vec3 pos;
    glm::vec3 target;
    glm::vec3 up;

    glm::mat4 ViewMatrix() const;
};

struct PlayerCamera {
    glm::vec3 pos;
    glm::vec3 up;

    f32 pitch;
    f32 yaw;
    f32 roll;

    glm::vec3 FacingDirection() const;
    glm::vec3 RightDirection() const;
    glm::vec3 UpDirection() const;
    glm::mat4 ViewMatrix() const;
};

struct Renderer {
    enum class ShaderType {
        None  = -1,
        Point = 0,
        Spot,
        Sun,
        Ambient,
    };

    struct SharedData {
        glm::mat4 mtx_vp;
        glm::vec3 pos_view;
    };

    // TODO: need to look more carefully at std140 layout rules
    static_assert(offsetof(SharedData, mtx_vp) == 0);
    static_assert(offsetof(SharedData, pos_view) == 4 * sizeof(glm::vec4));

    static constexpr usize SHARED_DATA_SIZE = sizeof(SharedData);
    static constexpr usize SHARED_DATA_SLOT = 0;

    static constexpr usize NUM_LIGHT_SHADERS  = 4; // point, spot, sun, ambient
    static constexpr usize NUM_SHADOW_SHADERS = 3; // point, spot, sun

    ShaderProgram dl_shader[NUM_LIGHT_SHADERS]  = {};
    ShaderProgram sv_shader[NUM_SHADOW_SHADERS] = {};

    glm::mat4 mtx_view;
    glm::vec3 pos_view;

    glm::mat4 mtx_vp; // cached by RenderPrepass

    u32 res_width = 1920, res_height = 1080;
    f32 fov = 70.0f;

    UBO shared_data;

    static constexpr f32 CLIP_NEAR = 0.1f;
    static constexpr f32 CLIP_FAR  = 100.0f;

    Renderer(bool opengl_logging = false);

    Renderer& Resolution(u32 width, u32 height);
    Renderer& FOV(f32 fov);

    Renderer& ViewPosition(const glm::vec3& pos);
    Renderer& ViewMatrix(const glm::mat4& mtx);

    Renderer& ClearColor(f32 red, f32 green, f32 blue);

    void RenderPrepass();
    void RenderLighting(const AmbientLight& light, const std::vector<Object>& objs);
    void RenderLighting(const PointLight& light, const std::vector<Object>& objs);
    void RenderLighting(const SpotLight& light, const std::vector<Object>& objs);
    void RenderLighting(const SunLight& light, const std::vector<Object>& objs);
};
