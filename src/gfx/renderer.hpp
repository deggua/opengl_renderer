#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "common.hpp"
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

enum class TextureType {
    Diffuse  = 0,
    Specular = 1,
    Normal   = 2,
};

// TODO: texture pool

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
};

// TODO: mesh pool
// TODO: separate Mesh from Object (e.g. pure geometry & indicies vs geometry + texture)
struct Mesh {
    size_t len;

    VAO vao;
    VBO vbo;
    EBO ebo;

    // TODO: OBJ loading, etc
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);

    void Draw() const;
};

struct Material {
    Texture2D diffuse;
    Texture2D specular;
    f32       gloss;

    Material(const Texture2D& diffuse, const Texture2D& specular, f32 gloss);

    void Use(ShaderProgram& sp) const;
};

struct Object {
    Mesh     mesh;
    Material mat;

    static std::vector<Object> LoadObjects(std::string_view file_path);

    // Object(const Mesh& mesh, const Material& mat);

    void Draw(ShaderProgram& sp) const;
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
