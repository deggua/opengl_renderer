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

    ShaderProgram dl_shader[4] = {};
    ShaderProgram sv_shader[3] = {};

    Renderer(bool opengl_logging = false);

    void Set_Resolution(u32 width, u32 height);

    void Set_ViewMatrix(const glm::mat4& mtx_view);
    void Set_ScreenMatrix(const glm::mat4& mtx_screen);
    void Set_ViewPosition(const glm::vec3& view_pos);

    void Clear(const glm::vec3& color);
    void RenderLighting(const AmbientLight& light, const std::vector<Object>& objs);
    void RenderLighting(const PointLight& light, const std::vector<Object>& objs);
    void RenderLighting(const SpotLight& light, const std::vector<Object>& objs);
    void RenderLighting(const SunLight& light, const std::vector<Object>& objs);
};
