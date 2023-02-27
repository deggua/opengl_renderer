#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "common.hpp"
#include "gfx/opengl.hpp"

// TODO: why not just use a vector?
struct DistanceFalloff {
    glm::vec3 k;
};

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

    DistanceFalloff falloff;

    f32 inner_cutoff; // dot(dir, inner_dir)
    f32 outer_cutoff; // dot(dit, outer_dir)
    f32 intensity;
};

struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;

    DistanceFalloff falloff;

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
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
};

struct Mesh {
    size_t len; // TODO: this has different meanings if you're using indices or vertices, not clear

    VAO vao;
    VBO vbo;
    EBO ebo;

    // TODO: OBJ loading, etc
    Mesh(const std::vector<Vertex>& vertices);

    void Draw() const;
};

struct Material {
    Texture2D diffuse;
    Texture2D specular;
    f32       gloss;
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
    glm::mat4 ViewMatrix() const;
};

struct Renderer {
    enum class ShaderType {
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
    void Render(const Mesh& mesh, const Material& mat, const Light& light);
};

Light CreateLight_Ambient(glm::vec3 color, f32 intensity);
Light CreateLight_Point(glm::vec3 pos, f32 range, glm::vec3 color, f32 intensity);
Light CreateLight_Spot(
    glm::vec3 pos,
    glm::vec3 dir,
    f32       range,
    f32       inner_theta_deg,
    f32       outer_theta_deg,
    glm::vec3 color,
    f32       intensity);
Light CreateLight_Sun(glm::vec3 dir, glm::vec3 color, f32 intensity);
