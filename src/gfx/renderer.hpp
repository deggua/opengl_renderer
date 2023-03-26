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
    static VAO  vao;
    static VBO  vbo;
    static bool is_vao_initialized;

    FullscreenQuad();
    void Draw() const;
};

// pack of data for render passes to use
struct RenderState {
    glm::mat4 mtx_vp;   // VP matrix         (world -> screen)
    glm::mat4 mtx_view; // View matrix       (world -> view)
    glm::mat4 mtx_proj; // Projection matrix (view -> screen)

    glm::vec3 pos_view; // View position (in world space)
};

// TODO: it's kind of dumb to compile some of the shaders multiple times since they don't change
// maybe we can use an asset cache to store compiled shaders
struct Renderer_AmbientLighting {
    Shader        vs, fs;
    ShaderProgram sp_light;

    Renderer_AmbientLighting();

    void Render(const AmbientLight& light, const std::vector<Object>& objs, const RenderState& rs);
};

struct Renderer_PointLighting {
    Shader        vs, fs;
    Shader        vs_shadow, gs_shadow, fs_shadow;
    ShaderProgram sp_light, sp_shadow;

    Renderer_PointLighting();

    void Render(const PointLight& light, const std::vector<Object>& objs, const RenderState& rs);
};

struct Renderer_SpotLighting {
    Shader        vs, fs;
    Shader        vs_shadow, gs_shadow, fs_shadow;
    ShaderProgram sp_light, sp_shadow;

    Renderer_SpotLighting();

    void Render(const SpotLight& light, const std::vector<Object>& objs, const RenderState& rs);
};

struct Renderer_SunLighting {
    Shader        vs, fs;
    Shader        vs_shadow, gs_shadow, fs_shadow;
    ShaderProgram sp_light, sp_shadow;

    Renderer_SunLighting();

    void Render(const SunLight& light, const std::vector<Object>& objs, const RenderState& rs);
};

struct Renderer_Skybox {
    Shader        vs, fs;
    ShaderProgram sp;

    Renderer_Skybox();

    void Render(const Skybox& sky, const RenderState& rs);
};

struct Renderer_SphericalBillboard {
    Shader        vs, fs;
    ShaderProgram sp;

    Renderer_SphericalBillboard();

    void Render(const std::vector<Sprite3D>& sprites, const RenderState& rs);
};

struct Renderer_PostFX {
    Shader         vs, fs;
    ShaderProgram  sp;
    FullscreenQuad quad;

    Renderer_PostFX();

    void Render(const TextureRT& src_hdr, const FBO& dst_sdr);
};

struct Renderer_Bloom {
    static constexpr f32   BLOOM_RADIUS        = 0.005f;
    static constexpr f32   BLOOM_STRENGTH      = 0.04f;
    static constexpr isize BLOOM_MIP_CHAIN_LEN = 6;

    ShaderProgram sp_upscale, sp_downscale, sp_final;

    glm::vec2 input_res;

    struct {
        glm::vec2 res;
        TextureRT tex;
    } mips[BLOOM_MIP_CHAIN_LEN];

    Shader vs, fs_upscale, fs_downscale, fs_final;

    FBO fbo;

    FullscreenQuad quad;

    bool mips_setup = false;

    Renderer_Bloom();

    void SetResolution(f32 width, f32 height);
    void Render(const TextureRT& src_hdr, const FBO& dst_hdr, f32 radius, f32 strength);
};

struct Renderer {
    static constexpr f32 CLIP_NEAR = 0.1f;
    static constexpr f32 CLIP_FAR  = 50.0f;

    // TODO: merge these? maybe not?
    RenderState rs;

    // used in the UBO for cross shader storage
    struct SharedData {
        glm::mat4 mtx_vp;
        glm::mat4 mtx_view;
        glm::mat4 mtx_proj;
        glm::vec3 pos_view;
    };

    // render passes
    Renderer_AmbientLighting    rp_ambient_lighting;
    Renderer_PointLighting      rp_point_lighting;
    Renderer_SpotLighting       rp_spot_lighting;
    Renderer_SunLighting        rp_sun_lighting;
    Renderer_Skybox             rp_skybox;
    Renderer_SphericalBillboard rp_spherical_billboard;
    Renderer_Bloom              rp_bloom;
    Renderer_PostFX             rp_postfx;

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

    Renderer(bool opengl_logging = false);

    Renderer& Resolution(u32 width, u32 height);
    Renderer& FOV(f32 fov);

    Renderer& ViewPosition(const glm::vec3& pos);
    Renderer& ViewMatrix(const glm::mat4& mtx);

    Renderer& ClearColor(f32 red, f32 green, f32 blue);

    void RenderPrepass();
    void RenderObjectLighting(const AmbientLight& light, const std::vector<Object>& objs);
    void RenderObjectLighting(const PointLight& light, const std::vector<Object>& objs);
    void RenderObjectLighting(const SpotLight& light, const std::vector<Object>& objs);
    void RenderObjectLighting(const SunLight& light, const std::vector<Object>& objs);
    void RenderSkybox(const Skybox& sky);
    void RenderSprite(const std::vector<Sprite3D>& sprites);
    void RenderScreen();
    void RenderBloom();
};
