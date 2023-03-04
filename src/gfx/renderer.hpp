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

struct Light {
    LightType type;

    union {
        AmbientLight ambient;
        PointLight   point;
        SpotLight    spot;
        SunLight     sun;
    };

    static Light Ambient(glm::vec3 color, f32 intensity);
    static Light Point(glm::vec3 pos, glm::vec3 color, f32 intensity);
    static Light
    Spot(glm::vec3 pos, glm::vec3 dir, f32 inner_theta_deg, f32 outer_theta_deg, glm::vec3 color, f32 intensity);
    static Light Sun(glm::vec3 dir, glm::vec3 color, f32 intensity);
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
        None    = -1,
        Ambient = 0,
        Point,
        Spot,
        Sun
    };

    ShaderProgram shaders[4] = {};

    Renderer();

    void Set_Resolution(u32 width, u32 height);
    void Set_NormalMatrix(const glm::mat3& mtx_normal);
    void Set_WorldMatrix(const glm::mat4& mtx_world);
    void Set_ViewMatrix(const glm::mat4& mtx_view);
    void Set_ScreenMatrix(const glm::mat4& mtx_screen);
    void Set_ViewPosition(const glm::vec3& view_pos);

    void Enable(GLenum setting);
    void Clear(const glm::vec3& color);
    void Render(const Object& obj, const Light& light);
};
