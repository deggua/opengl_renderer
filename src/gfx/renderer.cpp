#include "renderer.hpp"

#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"
#include "gfx/opengl.hpp"
#include "math/random.hpp"
#include "utils/profiling.hpp"
#include "utils/settings.hpp"

SHADER_FILE(Lighting_VS);
SHADER_FILE(Lighting_FS);
SHADER_FILE(ShadowVolume_VS);
SHADER_FILE(ShadowVolume_FS);
SHADER_FILE(ShadowVolume_GS);
SHADER_FILE(Skybox_FS);
SHADER_FILE(Skybox_VS);
SHADER_FILE(PostFX_VS);
SHADER_FILE(PostFX_Sharpen_FS);
SHADER_FILE(PostFX_Gamma_FS);
SHADER_FILE(PostFX_Tonemap_FS);
SHADER_FILE(SphericalBillboard_FS);
SHADER_FILE(SphericalBillboard_VS);
SHADER_FILE(Bloom_VS);
SHADER_FILE(BloomDownsample_FS);
SHADER_FILE(BloomUpsample_FS);
SHADER_FILE(BloomFinal_FS);

struct String {
    size_t      len;
    const char* str;

    constexpr String(const char* str_literal)
    {
        str = str_literal;
        len = std::char_traits<char>::length(str_literal);
    }
};

static constexpr String ShaderPreamble_Version = String("#version 450 core\n");
static constexpr String ShaderPreamble_Line    = String("#line 1\n");

static constexpr String ShaderPreamble_Light = String(
    "#define AMBIENT_LIGHT 0\n"
    "#define POINT_LIGHT   1\n"
    "#define SPOT_LIGHT    2\n"
    "#define SUN_LIGHT     3\n");

static constexpr String ShaderPreamble_LightType[] = {
    String("#define LIGHT_TYPE AMBIENT_LIGHT\n"),
    String("#define LIGHT_TYPE POINT_LIGHT\n"),
    String("#define LIGHT_TYPE SPOT_LIGHT\n"),
    String("#define LIGHT_TYPE SUN_LIGHT\n"),
};

static Shader CompileLightShader(GLenum shader_type, LightType type, const char* src, i32 len)
{
    const GLchar* source_fragments[] = {
        ShaderPreamble_Version.str,
        ShaderPreamble_Light.str,
        ShaderPreamble_LightType[(usize)type].str,
        ShaderPreamble_Line.str,
        src,
    };

    const GLint source_lens[] = {
        (GLint)ShaderPreamble_Version.len,
        (GLint)ShaderPreamble_Light.len,
        (GLint)ShaderPreamble_LightType[(usize)type].len,
        (GLint)ShaderPreamble_Line.len,
        (GLint)len,
    };

    return CompileShader(shader_type, lengthof(source_fragments), source_fragments, source_lens);
}

/* --- Ambient Light --- */
AmbientLight::AmbientLight(const glm::vec3& color, f32 intensity)
{
    this->color     = color;
    this->intensity = intensity;
}

AmbientLight& AmbientLight::Color(f32 r, f32 g, f32 b)
{
    this->color = glm::vec3(r, g, b);
    return *this;
}

AmbientLight& AmbientLight::Color(const glm::vec3& new_color)
{
    this->color = new_color;
    return *this;
}

glm::vec3 AmbientLight::Color() const
{
    return this->color;
}

AmbientLight& AmbientLight::Intensity(f32 new_intensity)
{
    this->intensity = new_intensity;
    return *this;
}

f32 AmbientLight::Intensity() const
{
    return this->intensity;
}

/* --- Sun Light --- */
SunLight::SunLight(const glm::vec3& dir, const glm::vec3& color, f32 intensity)
{
    this->dir       = glm::normalize(dir);
    this->color     = color;
    this->intensity = intensity;
}

SunLight& SunLight::Direction(const glm::vec3& new_dir)
{
    this->dir = glm::normalize(new_dir);
    return *this;
}

glm::vec3 SunLight::Direction() const
{
    return this->dir;
}

SunLight& SunLight::Color(f32 r, f32 g, f32 b)
{
    this->color = glm::vec3(r, g, b);
    return *this;
}

SunLight& SunLight::Color(const glm::vec3& new_color)
{
    this->color = new_color;
    return *this;
}

glm::vec3 SunLight::Color() const
{
    return this->color;
}

SunLight& SunLight::Intensity(f32 new_intensity)
{
    this->intensity = new_intensity;
    return *this;
}

/* --- Spot Light --- */
SpotLight::SpotLight(
    const glm::vec3& pos,
    const glm::vec3& dir,
    const glm::vec3& color,
    f32              inner_deg,
    f32              outer_deg,
    f32              intensity)
{
    this->pos          = pos;
    this->dir          = glm::normalize(dir);
    this->color        = color;
    this->inner_cutoff = glm::cos(glm::radians(inner_deg));
    this->outer_cutoff = glm::cos(glm::radians(outer_deg));
    this->intensity    = intensity;
}

SpotLight& SpotLight::Position(const glm::vec3& new_pos)
{
    this->pos = new_pos;
    return *this;
}

SpotLight& SpotLight::Position(f32 x, f32 y, f32 z)
{
    this->pos = glm::vec3(x, y, z);
    return *this;
}

glm::vec3 SpotLight::Position() const
{
    return this->pos;
}

SpotLight& SpotLight::Direction(const glm::vec3& new_dir)
{
    this->dir = glm::normalize(new_dir);
    return *this;
}

glm::vec3 SpotLight::Direction() const
{
    return this->dir;
}

SpotLight& SpotLight::Color(f32 r, f32 g, f32 b)
{
    this->color = glm::vec3(r, g, b);
    return *this;
}

SpotLight& SpotLight::Color(const glm::vec3& new_color)
{
    this->color = new_color;
    return *this;
}

glm::vec3 SpotLight::Color() const
{
    return this->color;
}

SpotLight& SpotLight::InnerCutoff(f32 angle_deg)
{
    this->inner_cutoff = glm::cos(glm::radians(angle_deg));
    return *this;
}

f32 SpotLight::InnerCutoff() const
{
    return glm::degrees(glm::acos(this->inner_cutoff));
}

SpotLight& SpotLight::OuterCutoff(f32 angle_deg)
{
    this->outer_cutoff = glm::cos(glm::radians(angle_deg));
    return *this;
}

f32 SpotLight::OuterCutoff() const
{
    return glm::degrees(glm::acos(this->outer_cutoff));
}

SpotLight& SpotLight::Cutoff(f32 inner_angle_deg, f32 outer_angle_deg)
{
    this->inner_cutoff = glm::cos(glm::radians(inner_angle_deg));
    this->outer_cutoff = glm::cos(glm::radians(outer_angle_deg));
    return *this;
}

SpotLight& SpotLight::Intensity(f32 new_intensity)
{
    this->intensity = new_intensity;
    return *this;
}

f32 SpotLight::Intensity() const
{
    return this->intensity;
}

/* --- Point Light --- */
PointLight::PointLight(const glm::vec3& pos, const glm::vec3& color, f32 intensity)
{
    this->pos       = pos;
    this->color     = color;
    this->intensity = intensity;
}

PointLight& PointLight::Position(const glm::vec3& new_pos)
{
    this->pos = new_pos;
    return *this;
}

PointLight& PointLight::Position(f32 x, f32 y, f32 z)
{
    this->pos = glm::vec3(x, y, z);
    return *this;
}

glm::vec3 PointLight::Position() const
{
    return this->pos;
}

PointLight& PointLight::Color(f32 r, f32 g, f32 b)
{
    this->color = glm::vec3(r, g, b);
    return *this;
}

PointLight& PointLight::Color(const glm::vec3& new_color)
{
    this->color = new_color;
    return *this;
}

glm::vec3 PointLight::Color() const
{
    return this->color;
}

PointLight& PointLight::Intensity(f32 new_intensity)
{
    this->intensity = new_intensity;
    return *this;
}

f32 PointLight::Intensity() const
{
    return this->intensity;
}

// Target Camera
glm::mat4 TargetCamera::ViewMatrix() const
{
    return glm::lookAt(this->pos, this->target, this->up);
}

// Player Camera
glm::vec3 PlayerCamera::FacingDirection() const
{
    glm::vec3 dir = {
        glm::cos(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch)),
    };

    return glm::normalize(dir);
}

glm::vec3 PlayerCamera::RightDirection() const
{
    glm::vec3 dir = {
        -glm::sin(glm::radians(this->yaw)),
        0,
        glm::cos(glm::radians(this->yaw)),
    };

    return glm::normalize(dir);
}

glm::vec3 PlayerCamera::UpDirection() const
{
    return this->up;
}

glm::mat4 PlayerCamera::ViewMatrix() const
{
    return glm::lookAt(this->pos, this->pos + this->FacingDirection(), this->up);
}

static void APIENTRY OpenGL_Debug_Callback(
    GLenum        source,
    GLenum        type,
    GLuint        id,
    GLenum        severity,
    GLsizei       length,
    const GLchar* message,
    const void*   user_param)
{
    (void)id;
    (void)length;
    (void)user_param;

    const auto src_str = [source]() {
        switch (source) {
            case GL_DEBUG_SOURCE_API:
                return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "WINDOW SYSTEM";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "SHADER COMPILER";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "THIRD PARTY";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "APPLICATION";
            case GL_DEBUG_SOURCE_OTHER:
                return "OTHER";
            default:
                return "???";
        }
    }();

    const auto type_str = [type]() {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                return "ERROR";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "DEPRECATED_BEHAVIOR";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "UNDEFINED_BEHAVIOR";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "PORTABILITY";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "PERFORMANCE";
            case GL_DEBUG_TYPE_MARKER:
                return "MARKER";
            case GL_DEBUG_TYPE_OTHER:
                return "OTHER";
            default:
                return "???";
        }
    }();

    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        LOG_INFO("Source: %s, Type: %s :: %s", src_str, type_str, message);
    } else if (severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOG_WARNING("Source: %s, Type: %s :: %s", src_str, type_str, message);
    } else {
        LOG_ERROR("Source: %s, Type: %s :: %s", src_str, type_str, message);
    }
}

static const glm::vec3 skybox_vertices[] = {
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    {-1.0f, -1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f,  1.0f,  1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f},
    {-1.0f, -1.0f,  1.0f},
    {-1.0f,  1.0f, -1.0f},
    { 1.0f,  1.0f, -1.0f},
    { 1.0f,  1.0f,  1.0f},
    { 1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f,  1.0f},
    {-1.0f,  1.0f, -1.0f},
    {-1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f, -1.0f},
    { 1.0f, -1.0f, -1.0f},
    {-1.0f, -1.0f,  1.0f},
    { 1.0f, -1.0f,  1.0f}
};

Skybox::Skybox(std::string_view skybox_path)
{
    // TODO: decouple the extension
    static const char* skybox_ext[] = {
        "/posx.jpg",
        "/negx.jpg",
        "/posy.jpg",
        "/negy.jpg",
        "/posz.jpg",
        "/negz.jpg",
    };

    std::array<std::string, 6> faces;

    for (usize ii = 0; ii < lengthof(skybox_ext); ii++) {
        faces[ii] = std::string(skybox_path) + skybox_ext[ii];
    }

    this->tex = TextureCubemap(faces);

    this->vao.Reserve();
    this->vao.Bind();

    this->vbo.Reserve();
    this->vbo.LoadData(sizeof(skybox_vertices), &skybox_vertices, GL_STATIC_DRAW);

    this->vbo.Bind();
    this->vao.SetAttribute(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);
}

void Skybox::Draw() const
{
    GL(glActiveTexture(GL_TEXTURE0));
    this->tex.Bind();

    this->vao.Bind();
    GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(skybox_vertices)));
}

/* --- FullscreenQuad --- */
static const glm::vec2 fullscreen_quad[] = {
  // positions   // texCoords
    {-1.0f,  1.0f},
    { 0.0f,  1.0f},
    {-1.0f, -1.0f},
    { 0.0f,  0.0f},
    { 1.0f, -1.0f},
    { 1.0f,  0.0f},

    {-1.0f,  1.0f},
    { 0.0f,  1.0f},
    { 1.0f, -1.0f},
    { 1.0f,  0.0f},
    { 1.0f,  1.0f},
    { 1.0f,  1.0f}
};

bool FullscreenQuad::is_vao_initialized = false;
VAO  FullscreenQuad::vao;
VBO  FullscreenQuad::vbo;

FullscreenQuad::FullscreenQuad()
{
    if (is_vao_initialized) {
        return;
    }

    this->vao.Reserve();
    this->vbo.Reserve();

    this->vbo.LoadData(sizeof(fullscreen_quad), &fullscreen_quad, GL_STATIC_DRAW);

    this->vbo.Bind();
    this->vao.SetAttribute(0, 2, GL_FLOAT, 2 * sizeof(glm::vec2), 0);
    this->vao.SetAttribute(1, 2, GL_FLOAT, 2 * sizeof(glm::vec2), sizeof(glm::vec2));

    this->is_vao_initialized = true;
}

void FullscreenQuad::Draw() const
{
    this->vao.Bind();
    GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(fullscreen_quad) / 2));
}

static constexpr f32 SHADOW_OFFSET_FACTOR = 0.025f;
static constexpr f32 SHADOW_OFFSET_UNITS  = 1.0f;

static void SetupDirectLightingPass(LightType light)
{
    // depth
    GL(glEnable(GL_DEPTH_TEST));
    GL(glDisable(GL_DEPTH_CLAMP));
    GL(glDepthMask(GL_TRUE));
    if (light != LightType::Ambient) {
        GL(glDepthFunc(GL_LEQUAL));
    } else {
        GL(glDepthFunc(GL_LESS));
    }

    // culling
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glFrontFace(GL_CCW));

    // pixel buffer
    GL(glEnable(GL_BLEND));
    GL(glBlendEquation(GL_FUNC_ADD));
    GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    if (light != LightType::Ambient) {
        GL(glBlendFunc(GL_ONE, GL_ONE));
    } else {
        GL(glBlendFunc(GL_ONE, GL_ZERO));
    }

    // stencil buffer
    if (light != LightType::Ambient) {
        GL(glEnable(GL_STENCIL_TEST));
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
    } else {
        GL(glDisable(GL_STENCIL_TEST));
    }

    // polygon offset
    GL(glDisable(GL_POLYGON_OFFSET_FILL));
}

static void SetupShadowLightingPass(LightType light)
{
    (void)light;

    // depth
    GL(glEnable(GL_DEPTH_TEST));
    GL(glEnable(GL_DEPTH_CLAMP));
    GL(glDepthMask(GL_FALSE));
    GL(glDepthFunc(GL_LESS));

    // culling
    GL(glDisable(GL_CULL_FACE));

    // pixel buffer
    GL(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));

    // stencil buffer
    GL(glEnable(GL_STENCIL_TEST));
    GL(glStencilFunc(GL_ALWAYS, 0, 0xFF));
    GL(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP));
    GL(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP));

    // polygon offset
    GL(glEnable(GL_POLYGON_OFFSET_FILL));
    GL(glPolygonOffset(SHADOW_OFFSET_FACTOR, SHADOW_OFFSET_UNITS));

    // clear stencil buffer
    GL(glClear(GL_STENCIL_BUFFER_BIT));
}

/* --- Renderer_AmbientLighting --- */
Renderer_AmbientLighting::Renderer_AmbientLighting()
{
    LOG_DEBUG("Compiling Ambient Lighting Vertex Shader");
    this->vs = CompileLightShader(
        GL_VERTEX_SHADER,
        LightType::Ambient,
        Lighting_VS.src,
        Lighting_VS.len);

    LOG_DEBUG("Compiling Ambient Lighting Fragment Shader");
    this->fs = CompileLightShader(
        GL_FRAGMENT_SHADER,
        LightType::Ambient,
        Lighting_FS.src,
        Lighting_FS.len);

    LOG_DEBUG("Linking Ambient Lighting Shaders");
    this->sp_light = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Ambient Lighting Shader Program = %u", this->sp_light.handle);

    LOG_DEBUG("Initializing Ambient Lighting Shader Program");
    this->sp_light.SetUniform("g_material.diffuse", 0);
    this->sp_light.SetUniform("g_material.specular", 1);
    this->sp_light.SetUniform("g_material.normal", 2);
}

void Renderer_AmbientLighting::Render(
    const AmbientLight&        light,
    const std::vector<Object>& objs,
    const RenderState&         rs)
{
    SetupDirectLightingPass(LightType::Ambient);
    this->sp_light.UseProgram();
    this->sp_light.SetUniform("g_light_source.color", light.color * light.intensity);

    for (const auto& obj : objs) {
        this->sp_light.SetUniform("g_mtx_world", obj.WorldMatrix());
        this->sp_light.SetUniform("g_mtx_normal", obj.NormalMatrix());
        this->sp_light.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

        obj.DrawVisual(this->sp_light);
    }
}

/* --- Renderer_PointLighting --- */
Renderer_PointLighting::Renderer_PointLighting()
{
    // direct lighting
    LOG_DEBUG("Compiling Point Lighting Vertex Shader");
    this->vs
        = CompileLightShader(GL_VERTEX_SHADER, LightType::Point, Lighting_VS.src, Lighting_VS.len);

    LOG_DEBUG("Compiling Point Lighting Fragment Shader");
    this->fs = CompileLightShader(
        GL_FRAGMENT_SHADER,
        LightType::Point,
        Lighting_FS.src,
        Lighting_FS.len);

    LOG_DEBUG("Linking Point Lighting Shaders");
    this->sp_light = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Point Lighting Shader Program = %u", this->sp_light.handle);

    LOG_DEBUG("Initializing Point Lighting Shader Program");
    this->sp_light.SetUniform("g_material.diffuse", 0);
    this->sp_light.SetUniform("g_material.specular", 1);
    this->sp_light.SetUniform("g_material.normal", 2);

    // shadow casting
    LOG_DEBUG("Compiling Point Lighting (Shadows) Vertex Shader");
    this->vs_shadow = CompileShader(GL_VERTEX_SHADER, ShadowVolume_VS.src, ShadowVolume_VS.len);

    LOG_DEBUG("Compiling Point Lighting (Shadows) Geometry Shader");
    this->gs_shadow = CompileLightShader(
        GL_GEOMETRY_SHADER,
        LightType::Point,
        ShadowVolume_GS.src,
        ShadowVolume_GS.len);

    LOG_DEBUG("Compiling Point Lighting (Shadows) Fragment Shader");
    this->fs_shadow = CompileShader(GL_FRAGMENT_SHADER, ShadowVolume_FS.src, ShadowVolume_FS.len);

    LOG_DEBUG("Linking Point Lighting (Shadows) Shaders");
    this->sp_shadow = LinkShaders(this->vs_shadow, this->gs_shadow, this->fs_shadow);
    LOG_DEBUG("Point Lighting (Shadows) Shader Program = %u", this->sp_shadow.handle);
}

void Renderer_PointLighting::Render(
    const PointLight&          light,
    const std::vector<Object>& objs,
    const RenderState&         rs)
{
    SetupShadowLightingPass(LightType::Point);
    this->sp_shadow.UseProgram();
    this->sp_shadow.SetUniform("g_light_source.pos", light.pos);

    for (const auto& obj : objs) {
        if (obj.CastsShadows()) {
            this->sp_shadow.SetUniform("g_mtx_world", obj.WorldMatrix());
            this->sp_shadow.SetUniform("g_mtx_normal", obj.NormalMatrix());
            this->sp_shadow.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

            obj.DrawShadow(this->sp_shadow);
        }
    }

    SetupDirectLightingPass(LightType::Point);
    this->sp_light.UseProgram();
    this->sp_light.SetUniform("g_light_source.pos", light.pos);
    this->sp_light.SetUniform("g_light_source.color", light.color * light.intensity);

    for (const auto& obj : objs) {
        this->sp_light.SetUniform("g_mtx_world", obj.WorldMatrix());
        this->sp_light.SetUniform("g_mtx_normal", obj.NormalMatrix());
        this->sp_light.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

        obj.DrawVisual(this->sp_light);
    }
}

/* --- Renderer_SpotLighting --- */
Renderer_SpotLighting::Renderer_SpotLighting()
{
    // direct lighting
    LOG_DEBUG("Compiling Spot Lighting Vertex Shader");
    this->vs
        = CompileLightShader(GL_VERTEX_SHADER, LightType::Spot, Lighting_VS.src, Lighting_VS.len);

    LOG_DEBUG("Compiling Spot Lighting Fragment Shader");
    this->fs
        = CompileLightShader(GL_FRAGMENT_SHADER, LightType::Spot, Lighting_FS.src, Lighting_FS.len);

    LOG_DEBUG("Linking Spot Lighting Shaders");
    this->sp_light = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Spot Lighting Shader Program = %u", this->sp_light.handle);

    LOG_DEBUG("Initializing Spot Lighting Shader Program");
    this->sp_light.SetUniform("g_material.diffuse", 0);
    this->sp_light.SetUniform("g_material.specular", 1);
    this->sp_light.SetUniform("g_material.normal", 2);

    // shadow casting
    LOG_DEBUG("Compiling Spot Lighting (Shadows) Vertex Shader");
    this->vs_shadow = CompileShader(GL_VERTEX_SHADER, ShadowVolume_VS.src, ShadowVolume_VS.len);

    LOG_DEBUG("Compiling Spot Lighting (Shadows) Geometry Shader");
    this->gs_shadow = CompileLightShader(
        GL_GEOMETRY_SHADER,
        LightType::Spot,
        ShadowVolume_GS.src,
        ShadowVolume_GS.len);

    LOG_DEBUG("Compiling Spot Lighting (Shadows) Fragment Shader");
    this->fs_shadow = CompileShader(GL_FRAGMENT_SHADER, ShadowVolume_FS.src, ShadowVolume_FS.len);

    LOG_DEBUG("Linking Spot Lighting (Shadows) Shaders");
    this->sp_shadow = LinkShaders(this->vs_shadow, this->gs_shadow, this->fs_shadow);
    LOG_DEBUG("Spot Lighting (Shadows) Shader Program = %u", this->sp_shadow.handle);
}

void Renderer_SpotLighting::Render(
    const SpotLight&           light,
    const std::vector<Object>& objs,
    const RenderState&         rs)
{
    SetupShadowLightingPass(LightType::Spot);
    this->sp_shadow.UseProgram();
    this->sp_shadow.SetUniform("g_light_source.pos", light.pos);
    this->sp_shadow.SetUniform("g_light_source.dir", light.dir);
    this->sp_shadow.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
    this->sp_shadow.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);

    for (const auto& obj : objs) {
        if (obj.CastsShadows()) {
            this->sp_shadow.SetUniform("g_mtx_world", obj.WorldMatrix());
            this->sp_shadow.SetUniform("g_mtx_normal", obj.NormalMatrix());
            this->sp_shadow.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

            obj.DrawShadow(this->sp_shadow);
        }
    }

    SetupDirectLightingPass(LightType::Spot);
    this->sp_light.UseProgram();
    this->sp_light.SetUniform("g_light_source.pos", light.pos);
    this->sp_light.SetUniform("g_light_source.dir", light.dir);
    this->sp_light.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
    this->sp_light.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);
    this->sp_light.SetUniform("g_light_source.color", light.color * light.intensity);

    for (const auto& obj : objs) {
        this->sp_light.SetUniform("g_mtx_world", obj.WorldMatrix());
        this->sp_light.SetUniform("g_mtx_normal", obj.NormalMatrix());
        this->sp_light.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

        obj.DrawVisual(this->sp_light);
    }
}

/* --- Renderer_SunLighting --- */
Renderer_SunLighting::Renderer_SunLighting()
{
    // direct lighting
    LOG_DEBUG("Compiling Sun Lighting Vertex Shader");
    this->vs
        = CompileLightShader(GL_VERTEX_SHADER, LightType::Sun, Lighting_VS.src, Lighting_VS.len);

    LOG_DEBUG("Compiling Sun Lighting Fragment Shader");
    this->fs
        = CompileLightShader(GL_FRAGMENT_SHADER, LightType::Sun, Lighting_FS.src, Lighting_FS.len);

    LOG_DEBUG("Linking Sun Lighting Shaders");
    this->sp_light = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Sun Lighting Shader Program = %u", this->sp_light.handle);

    LOG_DEBUG("Initializing Sun Lighting Shader Program");
    this->sp_light.SetUniform("g_material.diffuse", 0);
    this->sp_light.SetUniform("g_material.specular", 1);
    this->sp_light.SetUniform("g_material.normal", 2);

    // shadow casting
    LOG_DEBUG("Compiling Sun Lighting (Shadows) Vertex Shader");
    this->vs_shadow = CompileShader(GL_VERTEX_SHADER, ShadowVolume_VS.src, ShadowVolume_VS.len);

    LOG_DEBUG("Compiling Sun Lighting (Shadows) Geometry Shader");
    this->gs_shadow = CompileLightShader(
        GL_GEOMETRY_SHADER,
        LightType::Sun,
        ShadowVolume_GS.src,
        ShadowVolume_GS.len);

    LOG_DEBUG("Compiling Sun Lighting (Shadows) Fragment Shader");
    this->fs_shadow = CompileShader(GL_FRAGMENT_SHADER, ShadowVolume_FS.src, ShadowVolume_FS.len);

    LOG_DEBUG("Linking Sun Lighting (Shadows) Shaders");
    this->sp_shadow = LinkShaders(this->vs_shadow, this->gs_shadow, this->fs_shadow);
    LOG_DEBUG("Sun Lighting (Shadows) Shader Program = %u", this->sp_shadow.handle);
}

void Renderer_SunLighting::Render(
    const SunLight&            light,
    const std::vector<Object>& objs,
    const RenderState&         rs,
    Image2D&                   shadow_depth)
{
    SetupShadowLightingPass(LightType::Sun);
    this->sp_shadow.UseProgram();
    this->sp_shadow.SetUniform("g_light_source.dir", light.dir);

    shadow_depth.Bind(0);
    shadow_depth.Clear();
    GL(glMemoryBarrier(
        GL_SHADER_IMAGE_ACCESS_BARRIER_BIT)); // TODO: might not be necessary, I don't fully
                                              // understand whether I need `coherent` in the shader
                                              // or this here

    for (const auto& obj : objs) {
        if (obj.CastsShadows()) {
            this->sp_shadow.SetUniform("g_mtx_world", obj.WorldMatrix());
            this->sp_shadow.SetUniform("g_mtx_normal", obj.NormalMatrix());
            this->sp_shadow.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

            obj.DrawShadow(this->sp_shadow);
        }
    }

    SetupDirectLightingPass(LightType::Sun);
    this->sp_light.UseProgram();
    this->sp_light.SetUniform("g_light_source.dir", light.dir);
    this->sp_light.SetUniform("g_light_source.color", light.color * light.intensity);

    for (const auto& obj : objs) {
        this->sp_light.SetUniform("g_mtx_world", obj.WorldMatrix());
        this->sp_light.SetUniform("g_mtx_normal", obj.NormalMatrix());
        this->sp_light.SetUniform("g_mtx_wvp", rs.mtx_vp * obj.WorldMatrix());

        obj.DrawVisual(this->sp_light);
    }
}

/* --- Renderer_Skybox --- */
Renderer_Skybox::Renderer_Skybox()
{
    LOG_DEBUG("Compiling Skybox Vertex Shader");
    this->vs = CompileShader(GL_VERTEX_SHADER, Skybox_VS.src, Skybox_VS.len);

    LOG_DEBUG("Compiling Skybox Fragment Shader");
    this->fs = CompileShader(GL_FRAGMENT_SHADER, Skybox_FS.src, Skybox_FS.len);

    LOG_DEBUG("Linking Skybox Shaders");
    this->sp = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Skybox Shader Program = %u", this->sp.handle);

    LOG_DEBUG("Initializing Skybox Shader Program");
    this->sp.SetUniform("g_skybox", 0);
}

void Renderer_Skybox::Render(const Skybox& sky, const RenderState& rs)
{
    // TODO: be more explicit
    GL(glDisable(GL_BLEND));
    GL(glDisable(GL_STENCIL_TEST));
    GL(glDepthFunc(GL_LEQUAL));

    glm::mat4 vp_fixed = rs.mtx_proj * glm::mat4(glm::mat3(rs.mtx_view));

    this->sp.UseProgram();
    this->sp.SetUniform("g_mtx_vp_fixed", vp_fixed);

    sky.Draw();
}

/* --- Renderer_SphericalBillboard --- */
Renderer_SphericalBillboard::Renderer_SphericalBillboard()
{
    LOG_DEBUG("Compiling Spherical Billboard Vertex Shader");
    this->vs
        = CompileShader(GL_FRAGMENT_SHADER, SphericalBillboard_FS.src, SphericalBillboard_FS.len);

    LOG_DEBUG("Compiling Spherical Billboard Fragment Shader");
    this->fs
        = CompileShader(GL_VERTEX_SHADER, SphericalBillboard_VS.src, SphericalBillboard_VS.len);

    LOG_DEBUG("Linking Spherical Billboard Shaders");
    this->sp = LinkShaders(this->vs, this->fs);
    LOG_DEBUG("Spherical Billboard Shader Program = %u", this->sp.handle);

    LOG_DEBUG("Initializing Spherical Billboard Shader Program");
    this->sp.SetUniform("g_sprite", 0);
}

// TODO: cleanup naming, should be something like SphericalBillboard3D
// fixed size regardless of distance should be SphericalBillboard2D, etc.
// TODO: some sprites may be emissive (light flares), others may not be (smoke), needs better
// handling
void Renderer_SphericalBillboard::Render(
    const std::vector<Sprite3D>& sprites,
    const RenderState&           rs)
{
    // TODO: be more explicit
    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));
    GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    GL(glEnable(GL_CULL_FACE));

    GL(glEnable(GL_DEPTH_TEST));
    GL(glDepthMask(GL_FALSE));
    GL(glDepthFunc(GL_LESS));

    GL(glDisable(GL_STENCIL_TEST));

    this->sp.UseProgram();
    for (const auto& sprite : sprites) {
        // TODO: why not apply the matrix fixup here?
        this->sp.SetUniform("g_mtx_wv", rs.mtx_view * sprite.WorldMatrix());
        this->sp.SetUniform("g_scale", sprite.Scale());
        this->sp.SetUniform("g_intensity", sprite.Intensity());
        this->sp.SetUniform("g_tint", sprite.Tint());

        sprite.Draw(this->sp);
    }

    // TODO: this shouldn't be required, other passes should be explicit about the depth mask
    GL(glDepthMask(GL_TRUE));
}

/* --- Renderer_Bloom --- */
Renderer_Bloom::Renderer_Bloom()
{
    LOG_DEBUG("Compiling Bloom Vertex Shader");
    this->vs = CompileShader(GL_VERTEX_SHADER, Bloom_VS.src, Bloom_VS.len);

    LOG_DEBUG("Compiling Bloom Upscale Fragment Shader");
    this->fs_upscale
        = CompileShader(GL_FRAGMENT_SHADER, BloomUpsample_FS.src, BloomUpsample_FS.len);

    LOG_DEBUG("Linking Bloom Upscale Shaders");
    this->sp_upscale = LinkShaders(this->vs, this->fs_upscale);
    LOG_DEBUG("Bloom Upscale Shader Program = %u", this->sp_upscale.handle);

    LOG_DEBUG("Initializng Bloom Upscale Shader Program");
    this->sp_upscale.SetUniform("g_tex_input", 0);
    this->sp_upscale.SetUniform("g_radius", BLOOM_RADIUS);

    LOG_DEBUG("Compiling Bloom Downscale Fragment Shader");
    this->fs_downscale
        = CompileShader(GL_FRAGMENT_SHADER, BloomDownsample_FS.src, BloomDownsample_FS.len);

    LOG_DEBUG("Linking Bloom Downscale Shaders");
    this->sp_downscale = LinkShaders(this->vs, this->fs_downscale);
    LOG_DEBUG("Bloom Downscale Shader Program = %u", this->sp_downscale.handle);

    LOG_DEBUG("Initializing Bloom Downscale Shader Program");
    this->sp_downscale.SetUniform("g_tex_input", 0);
    this->sp_downscale.SetUniform("g_resolution", glm::vec2(0, 0));

    LOG_DEBUG("Compiling Bloom Final Fragment Shader");
    this->fs_final = CompileShader(GL_FRAGMENT_SHADER, BloomFinal_FS.src, BloomFinal_FS.len);

    LOG_DEBUG("Linking Bloom Final Shaders");
    this->sp_final = LinkShaders(this->vs, this->fs_final);
    LOG_DEBUG("Bloom Final Shader Program = %u", this->sp_final.handle);

    LOG_DEBUG("Initializing Bloom Final Shader Program");
    this->sp_final.SetUniform("g_tex_hdr", 0);
    this->sp_final.SetUniform("g_tex_bloom", 1);
    this->sp_final.SetUniform("g_bloom_strength", 0.04f);

    LOG_DEBUG("Creating Bloom FBO");
    this->fbo.Reserve();
}

void Renderer_Bloom::SetResolution(f32 width, f32 height)
{
    this->input_res = {width, height};
    glm::vec2 res   = {width, height};
    for (usize ii = 0; ii < BLOOM_MIP_CHAIN_LEN; ii++) {
        if (this->mips_setup) {
            this->mips[ii].tex.Delete();
        }

        res *= 0.5f;
        this->mips[ii].tex.Reserve();
        this->mips[ii].tex.Setup(GL_R11F_G11F_B10F, (GLsizei)res.x, (GLsizei)res.y);
        this->mips[ii].res = res;
    }

    this->fbo.Attach(this->mips[0].tex, GL_COLOR_ATTACHMENT0);
    this->fbo.CheckComplete();

    this->mips_setup = true;
}

void Renderer_Bloom::Render(const TextureRT& src_hdr, const FBO& dst_hdr, f32 radius, f32 strength)
{
    GL(glDisable(GL_DEPTH_TEST));

    GL(glEnable(GL_BLEND));
    GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE));
    GL(glBlendFunc(GL_ONE, GL_ZERO));
    GL(glBlendEquation(GL_FUNC_ADD));

    GL(glDisable(GL_STENCIL_TEST));

    // src_hdr is the input texture for first iteration of downsample
    src_hdr.Bind(GL_TEXTURE0);

    // internal FBO is the output target until the final upscale stage
    this->fbo.Bind();

    this->sp_downscale.UseProgram();
    this->sp_downscale.SetUniform("g_resolution", this->input_res);
    this->sp_downscale.SetUniform("g_mip", 0);

    // downsample passes
    for (isize ii = 0; ii < BLOOM_MIP_CHAIN_LEN; ii++) {
        // color attachment is the mip target, clear it before rendering to it
        GL(glViewport(0, 0, (GLsizei)this->mips[ii].res.x, (GLsizei)this->mips[ii].res.y));
        this->fbo.Attach(this->mips[ii].tex, GL_COLOR_ATTACHMENT0);
        GL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
        GL(glClear(GL_COLOR_BUFFER_BIT));

        this->quad.Draw();

        // for next iter
        // input res is this stage's res
        this->sp_downscale.SetUniform("g_resolution", this->mips[ii].res);
        this->sp_downscale.SetUniform("g_mip", (int)(ii + 1));
        // input texture is this stage's tex
        this->mips[ii].tex.Bind(GL_TEXTURE0);
    }

    // now we upsample and additively blend backwards
    GL(glBlendFunc(GL_ONE, GL_ONE));

    this->sp_upscale.UseProgram();
    this->sp_upscale.SetUniform("g_radius", radius);

    // upsample passes
    for (isize ii = BLOOM_MIP_CHAIN_LEN - 1; ii > 0; ii--) {
        this->mips[ii].tex.Bind(GL_TEXTURE0);
        GL(glViewport(0, 0, (GLsizei)this->mips[ii - 1].res.x, (GLsizei)this->mips[ii - 1].res.y));
        this->fbo.Attach(this->mips[ii - 1].tex, GL_COLOR_ATTACHMENT0);

        this->quad.Draw();
    }

    // final pass takes the highest res mip and mixes it with the input image and writes to the
    // output FBO
    GL(glBlendFunc(GL_ONE, GL_ZERO));

    GL(glViewport(0, 0, (GLsizei)this->input_res.x, (GLsizei)this->input_res.y));
    src_hdr.Bind(GL_TEXTURE0);

    this->mips[0].tex.Bind(GL_TEXTURE1);

    this->sp_final.UseProgram();
    this->sp_final.SetUniform("g_bloom_strength", strength);

    dst_hdr.Bind();

    this->quad.Draw();
}

/* --- Renderer_PostFX --- */
Renderer_PostFX::Renderer_PostFX()
{
    LOG_DEBUG("Compiling PostFX Vertex Shader");
    this->vs = CompileShader(GL_VERTEX_SHADER, PostFX_VS.src, PostFX_VS.len);

    // Sharpening
    LOG_DEBUG("Compiling PostFX Sharpen Fragment Shader");
    this->fs_sharpen
        = CompileShader(GL_FRAGMENT_SHADER, PostFX_Sharpen_FS.src, PostFX_Sharpen_FS.len);

    LOG_DEBUG("Linking PostFX Sharpen Shaders");
    this->sp_sharpen = LinkShaders(this->vs, this->fs_sharpen);
    LOG_DEBUG("PostFX Sharpen Shader Program = %u", this->sp_sharpen.handle);

    LOG_DEBUG("Initializing PostFX Sharpen Shader Program");
    this->sp_sharpen.SetUniform("g_screen", 0);

    // Gamma correction
    LOG_DEBUG("Compiling PostFX Gamma Fragment Shader");
    this->fs_gamma = CompileShader(GL_FRAGMENT_SHADER, PostFX_Gamma_FS.src, PostFX_Gamma_FS.len);

    LOG_DEBUG("Linking PostFX Gamma Shaders");
    this->sp_gamma = LinkShaders(this->vs, this->fs_gamma);
    LOG_DEBUG("PostFX Gamma Shader Program = %u", this->sp_gamma.handle);

    LOG_DEBUG("Initializing PostFX Gamma Shader Program");
    this->sp_gamma.SetUniform("g_screen", 0);

    // Tonemapping
    LOG_DEBUG("Compiling PostFX Tonemap Fragment Shader");
    this->fs_tonemap
        = CompileShader(GL_FRAGMENT_SHADER, PostFX_Tonemap_FS.src, PostFX_Tonemap_FS.len);

    LOG_DEBUG("Linking PostFX Tonemapping Shaders");
    this->sp_tonemap = LinkShaders(this->vs, this->fs_tonemap);
    LOG_DEBUG("PostFX Tonemapping Program = %u", this->sp_tonemap.handle);

    LOG_DEBUG("Initializing PostFX Shader Program");
    this->sp_tonemap.SetUniform("g_screen", 0);
}

void Renderer_PostFX::RenderTonemap(const TextureRT& src, const FBO& dst, GLuint tonemapper)
{
    dst.Bind();

    GL(glDisable(GL_DEPTH_TEST));
    GL(glDisable(GL_BLEND));
    GL(glDisable(GL_STENCIL_TEST));
    GL(glClear(GL_COLOR_BUFFER_BIT));

    this->sp_tonemap.UseProgram();
    this->sp_tonemap.SetUniform("g_tonemapper", tonemapper);

    src.Bind(GL_TEXTURE0);
    this->quad.Draw();
}

void Renderer_PostFX::RenderSharpen(
    const TextureRT& src,
    const FBO&       dst,
    glm::vec2        screen_resolution,
    f32              strength)
{
    dst.Bind();

    GL(glDisable(GL_DEPTH_TEST));
    GL(glDisable(GL_BLEND));
    GL(glDisable(GL_STENCIL_TEST));
    GL(glClear(GL_COLOR_BUFFER_BIT));

    this->sp_sharpen.UseProgram();
    this->sp_sharpen.SetUniform("g_resolution", screen_resolution);
    this->sp_sharpen.SetUniform("g_strength", strength);

    src.Bind(GL_TEXTURE0);
    this->quad.Draw();
}

void Renderer_PostFX::RenderGammaCorrect(const TextureRT& src, const FBO& dst, f32 gamma)
{
    dst.Bind();

    GL(glDisable(GL_DEPTH_TEST));
    GL(glDisable(GL_BLEND));
    GL(glDisable(GL_STENCIL_TEST));
    GL(glClear(GL_COLOR_BUFFER_BIT));

    this->sp_gamma.UseProgram();
    this->sp_gamma.SetUniform("g_gamma", gamma);

    src.Bind(GL_TEXTURE0);
    this->quad.Draw();
}

/* --- Renderer --- */
static void RenderInit(void)
{
    TexturePool.LoadStatic(DefaultTexture_Diffuse, Texture2D(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)));
    TexturePool.LoadStatic(DefaultTexture_Specular, Texture2D(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));
    TexturePool.LoadStatic(DefaultTexture_Normal, Texture2D(glm::vec4(0.5f, 0.5f, 1.0f, 1.0f)));

    Random_Seed_HighEntropy();

    GL(glEnable(GL_MULTISAMPLE));
}

Renderer::Renderer(bool opengl_logging)
{
    // TODO: synchronous is slow, not that big of an issue for now
    // TODO: this functionality doesn't exist in OpenGL 3.3
    if (opengl_logging) {
        GL(glEnable(GL_DEBUG_OUTPUT));
        GL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
        GL(glDebugMessageCallback(OpenGL_Debug_Callback, nullptr));
        GL(glDebugMessageControl(
            GL_DONT_CARE,
            GL_DONT_CARE,
            GL_DEBUG_SEVERITY_NOTIFICATION,
            0,
            nullptr,
            GL_FALSE));
    }

    // exterior render initialization
    RenderInit();

    // setup internal render target for MSAA
    this->msaa.fbo.Reserve();
    this->msaa.depth_stencil.Reserve();
    this->msaa.color.Reserve();

    this->msaa.depth_stencil.CreateStorage(
        GL_DEPTH24_STENCIL8,
        settings.msaa_samples,
        this->res_width,
        this->res_height);
    this->msaa.color
        .CreateStorage(GL_R11F_G11F_B10F, settings.msaa_samples, this->res_width, this->res_height);

    this->msaa.fbo.Attach(this->msaa.depth_stencil, GL_DEPTH_STENCIL_ATTACHMENT);
    this->msaa.fbo.Attach(this->msaa.color, GL_COLOR_ATTACHMENT0);
    this->msaa.fbo.CheckComplete();

    // setup internal render target for postprocessing
    for (usize ii = 0; ii < lengthof(this->post); ii++) {
        this->post[ii].fbo.Reserve();
        this->post[ii].depth_stencil.Reserve();

        this->post[ii].color.Reserve();
        this->post[ii]
            .depth_stencil.CreateStorage(GL_DEPTH24_STENCIL8, 1, this->res_width, this->res_height);
        this->post[ii].color.Setup(GL_R11F_G11F_B10F, this->res_width, this->res_height);

        this->post[ii].fbo.Attach(this->post[ii].depth_stencil, GL_DEPTH_STENCIL_ATTACHMENT);
        this->post[ii].fbo.Attach(this->post[ii].color, GL_COLOR_ATTACHMENT0);
        this->post[ii].fbo.CheckComplete();
    }

    // setup the shared UBO
    this->shared_data.Reserve(sizeof(SharedData));
    this->shared_data.BindSlot(0);

    // initial bloom setup
    this->rp_bloom.SetResolution(this->res_width, this->res_height);

    // initial shadow depth Image2D
    this->shadow_depth.Reserve(GL_R32F, this->res_width, this->res_height);
}

Renderer::Simple_RT& Renderer::GetRenderSource()
{
    return this->post[(this->post_target + 1) % lengthof(post)];
}

Renderer::Simple_RT& Renderer::GetRenderTarget()
{
    return this->post[this->post_target];
}

void Renderer::AdvanceRenderTarget()
{
    this->post_target = (this->post_target + 1) % lengthof(post);
}

Renderer& Renderer::Resolution(u32 width, u32 height)
{
    if (res_width == width && res_height == height) {
        return *this;
    }

    this->res_width  = width;
    this->res_height = height;

    this->msaa.depth_stencil.CreateStorage(
        GL_DEPTH24_STENCIL8,
        settings.msaa_samples,
        this->res_width,
        this->res_height);
    this->msaa.color.CreateStorage(GL_R11F_G11F_B10F, settings.msaa_samples, width, height);

    for (usize ii = 0; ii < lengthof(this->post); ii++) {
        this->post[ii].depth_stencil.CreateStorage(GL_DEPTH24_STENCIL8, 1, width, height);
        this->post[ii].color.Setup(GL_R11F_G11F_B10F, width, height);
    }

    this->rp_bloom.SetResolution(width, height);

    this->shadow_depth.Delete();
    this->shadow_depth.Reserve(GL_R32F, width, height);

    GL(glViewport(0, 0, width, height));

    return *this;
}

Renderer& Renderer::FOV(f32 new_fov)
{
    this->fov = new_fov;
    return *this;
}

Renderer& Renderer::ViewPosition(const glm::vec3& pos)
{
    this->rs.pos_view = pos;
    return *this;
}

Renderer& Renderer::ViewMatrix(const glm::mat4& mtx)
{
    this->rs.mtx_view = mtx;
    return *this;
}

Renderer& Renderer::ClearColor(f32 red, f32 green, f32 blue)
{
    GL(glClearColor(red, green, blue, 1.0f));
    return *this;
}

void Renderer::StartRender()
{
    PROFILE_FUNCTION();

    // cache the VP matrix for this render pass
    f32       aspect   = (f32)res_width / (f32)res_height;
    glm::mat4 mtx_proj = glm::perspective(glm::radians(this->fov), aspect, CLIP_NEAR, CLIP_FAR);
    this->rs.mtx_proj  = mtx_proj;
    this->rs.mtx_vp    = mtx_proj * this->rs.mtx_view;

    // Update UBO for VP matrix and View Position
    SharedData tmp = {
        this->rs.mtx_vp,
        this->rs.mtx_view,
        this->rs.mtx_proj,
        this->rs.pos_view,
    };
    this->shared_data.SubData(0, sizeof(SharedData), &tmp);

    // bind the internal frame target and clear the screen
    this->msaa.fbo.Bind();
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

// TODO: there should be a better way to organize these, they're very similar

void Renderer::RenderObjectLighting(const AmbientLight& light, const std::vector<Object>& objs)
{
    PROFILE_FUNCTION();
    this->rp_ambient_lighting.Render(light, objs, this->rs);
}

void Renderer::RenderObjectLighting(const PointLight& light, const std::vector<Object>& objs)
{
    PROFILE_FUNCTION();
    this->rp_point_lighting.Render(light, objs, this->rs);
}

void Renderer::RenderObjectLighting(const SpotLight& light, const std::vector<Object>& objs)
{
    PROFILE_FUNCTION();
    this->rp_spot_lighting.Render(light, objs, this->rs);
}

void Renderer::RenderObjectLighting(const SunLight& light, const std::vector<Object>& objs)
{
    PROFILE_FUNCTION();
    this->rp_sun_lighting.Render(light, objs, this->rs, this->shadow_depth);
}

void Renderer::RenderSkybox(const Skybox& sky)
{
    PROFILE_FUNCTION();
    this->rp_skybox.Render(sky, this->rs);
}

// TODO: naming
void Renderer::RenderSprites(const std::vector<Sprite3D>& sprites)
{
    PROFILE_FUNCTION();
    this->rp_spherical_billboard.Render(sprites, this->rs);
}

void Renderer::FinishGeometry()
{
    PROFILE_FUNCTION();

    // blit the MSAA FBO to the single sample FBO
    GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, this->msaa.fbo.handle));
    GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->GetRenderTarget().fbo.handle));
    GL(glBlitFramebuffer(
        0,
        0,
        this->res_width,
        this->res_height,
        0,
        0,
        this->res_width,
        this->res_height,
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST));

    this->AdvanceRenderTarget();
}

void Renderer::RenderBloom(f32 radius, f32 strength)
{
    PROFILE_FUNCTION();

    Simple_RT& src = this->GetRenderSource();
    Simple_RT& dst = this->GetRenderTarget();
    this->rp_bloom.Render(src.color, dst.fbo, radius, strength);

    this->AdvanceRenderTarget();
}

void Renderer::RenderTonemap(GLuint tonemapper)
{
    PROFILE_FUNCTION();

    Simple_RT& src = this->GetRenderSource();
    Simple_RT& dst = this->GetRenderTarget();
    this->rp_postfx.RenderTonemap(src.color, dst.fbo, tonemapper);

    this->AdvanceRenderTarget();
}

void Renderer::RenderSharpening(f32 strength)
{
    PROFILE_FUNCTION();

    Simple_RT& src = this->GetRenderSource();
    Simple_RT& dst = this->GetRenderTarget();
    this->rp_postfx
        .RenderSharpen(src.color, dst.fbo, {this->res_width, this->res_height}, strength);

    this->AdvanceRenderTarget();
}

// TODO: should gamma be a parameter to this function, or part of the renderer's state?
void Renderer::FinishRender(f32 gamma)
{
    PROFILE_FUNCTION();

    FBO        default_fbo = FBO();
    Simple_RT& src         = this->GetRenderSource();
    this->rp_postfx.RenderGammaCorrect(src.color, default_fbo, gamma);
}
