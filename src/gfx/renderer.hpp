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
    f32       intensity;

    AmbientLight(const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f), f32 intensity = 1.0f);

    AmbientLight& Color(f32 r, f32 g, f32 b);
    AmbientLight& Color(const glm::vec3& color);
    glm::vec3     Color() const;

    AmbientLight& Intensity(f32 intensity);
    f32           Intensity() const;
};

struct SunLight {
    glm::vec3 dir; // must be pre-normalized
    glm::vec3 color;
    f32       intensity;

    SunLight(
        const glm::vec3& dir       = glm::vec3(0.0f, -1.0f, 0.0f),
        const glm::vec3& color     = glm::vec3(1.0f, 1.0f, 1.0f),
        f32              intensity = 1.0f);

    SunLight& Direction(const glm::vec3& dir);
    glm::vec3 Direction() const;

    SunLight& Color(f32 r, f32 g, f32 b);
    SunLight& Color(const glm::vec3& color);
    glm::vec3 Color() const;

    SunLight& Intensity(f32 intensity);
    f32       Intensity() const;
};

struct SpotLight {
    glm::vec3 pos;
    glm::vec3 dir; // must be pre-normalized
    glm::vec3 color;
    f32       inner_cutoff; // dot(dir, inner_dir) or cos(angle)
    f32       outer_cutoff; // dot(dit, outer_dir) or cos(angle)
    f32       intensity;

    SpotLight(
        const glm::vec3& pos       = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& dir       = glm::vec3(0.0f, 0.0f, -1.0f),
        const glm::vec3& color     = glm::vec3(1.0f, 1.0f, 1.0f),
        f32              inner_deg = cosf(glm::radians(30.0f)),
        f32              outer_deg = cosf(glm::radians(45.0f)),
        f32              intensity = 1.0f);

    SpotLight& Position(const glm::vec3& pos);
    SpotLight& Position(f32 x, f32 y, f32 z);
    glm::vec3  Position() const;

    SpotLight& Direction(const glm::vec3& dir);
    glm::vec3  Direction() const;

    SpotLight& Color(f32 r, f32 g, f32 b);
    SpotLight& Color(const glm::vec3& color);
    glm::vec3  Color() const;

    SpotLight& InnerCutoff(f32 angle_deg);
    f32        InnerCutoff() const;

    SpotLight& OuterCutoff(f32 angle_deg);
    f32        OuterCutoff() const;

    SpotLight& Cutoff(f32 inner_angle_deg, f32 outer_angle_deg);

    SpotLight& Intensity(f32 intensity);
    f32        Intensity() const;
};

struct PointLight {
    glm::vec3 pos;
    glm::vec3 color;
    f32       intensity;

    PointLight(
        const glm::vec3& pos       = glm::vec3(0.0f, 0.0f, 0.0f),
        const glm::vec3& color     = glm::vec3(1.0f, 1.0f, 1.0f),
        f32              intensity = 1.0f);

    PointLight& Position(const glm::vec3& pos);
    PointLight& Position(f32 x, f32 y, f32 z);
    glm::vec3   Position() const;

    PointLight& Color(f32 r, f32 g, f32 b);
    PointLight& Color(const glm::vec3& color);
    glm::vec3   Color() const;

    PointLight& Intensity(f32 intensity);
    f32         Intensity() const;
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

struct Skybox {
    TextureCubemap tex;
    VAO            vao;
    VBO            vbo;

    Skybox(std::string_view skybox_path);
    void Draw() const;
};

struct FullscreenQuad {
    VAO vao;
    VBO vbo;

    FullscreenQuad();
    void Draw() const;
};

struct Renderer {
    enum class ShaderType {
        None  = -1,
        Point = 0,
        Spot,
        Sun,
        Ambient,
    };

    // used in the UBO for cross shader storage
    struct SharedData {
        glm::mat4 mtx_vp;
        glm::mat4 mtx_view;
        glm::mat4 mtx_proj;
        glm::vec3 pos_view;
    };

    static constexpr usize SHARED_DATA_SIZE = sizeof(SharedData);
    static constexpr usize SHARED_DATA_SLOT = 0;

    static constexpr usize NUM_LIGHT_SHADERS  = 4; // point, spot, sun, ambient
    static constexpr usize NUM_SHADOW_SHADERS = 3; // point, spot, sun

    static constexpr f32 SHADOW_OFFSET_FACTOR = 0.025f;
    static constexpr f32 SHADOW_OFFSET_UNITS  = 1.0f;

    static constexpr f32 CLIP_NEAR = 0.1f;
    static constexpr f32 CLIP_FAR  = 50.0f;

    ShaderProgram dl_shader[NUM_LIGHT_SHADERS]  = {};
    ShaderProgram sv_shader[NUM_SHADOW_SHADERS] = {};
    ShaderProgram sky_shader;
    ShaderProgram pp_shader;
    ShaderProgram bb_spherical_shader;

    glm::mat4 mtx_view;
    glm::mat4 mtx_proj;
    glm::vec3 pos_view;

    glm::mat4 mtx_vp; // cached by RenderPrepass

    u32 res_width = 1920, res_height = 1080;
    f32 fov = 90.0f;

    UBO shared_data;

    struct {
        FBO fbo;
        RBO depth_stencil;
        RBO color;
    } msaa;

    struct {
        FBO       fbo;
        RBO       depth_stencil;
        TextureRT color;
    } post;

    FullscreenQuad rt_quad;

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
    void RenderSkybox(const Skybox& sky);
    void RenderSprite(const Sprite3D& sprite);
    void RenderScreen();
};
