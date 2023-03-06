#include "renderer.hpp"

#include <stdio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"
#include "gfx/opengl.hpp"

SHADER_FILE(VS_Common);
SHADER_FILE(FS_Lighting);
SHADER_FILE(FS_ShadowVolume);
SHADER_FILE(GS_ShadowVolume);
SHADER_FILE(VS_ShadowVolume);

struct String {
    size_t      len;
    const char* str;

    constexpr String(const char* str_literal)
    {
        str = str_literal;
        len = std::char_traits<char>::length(str_literal);
    }
};

static constexpr String ShaderPreamble_Version = String("#version 330 core\n");
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

static Shader CompileLightingShader(GLenum shader_type, LightType type, const char* src, i32 len)
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

Light Light::Ambient(glm::vec3 color, f32 intensity)
{
    Light light = {
        .type = LightType::Ambient,
        .ambient = {
            .color = color,
            .intensity = intensity,
        },
    };

    return light;
}

Light Light::Sun(glm::vec3 dir, glm::vec3 color, f32 intensity)
{
    Light light = {
        .type = LightType::Sun,
        .sun = {
            .dir = glm::normalize(dir),
            .color = color,
            .intensity = intensity,
        },
    };

    return light;
}

Light Light::Spot(
    glm::vec3 pos,
    glm::vec3 dir,
    f32       inner_theta_deg,
    f32       outer_theta_deg,
    glm::vec3 color,
    f32       intensity)
{
    Light light = {
        .type = LightType::Spot,
        .spot = {
            .pos     = pos,
            .dir     = glm::normalize(dir),
            .color   = color,
            .inner_cutoff = glm::cos(glm::radians(inner_theta_deg)),
            .outer_cutoff = glm::cos(glm::radians(outer_theta_deg)),
            .intensity = intensity,
        },
    };

    return light;
}

Light Light::Point(glm::vec3 pos, glm::vec3 color, f32 intensity)
{
    Light light = {
        .type = LightType::Point,
        .point = {
            .pos = pos,
            .color = color,
            .intensity = intensity,
        },
    };

    return light;
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

    const auto severity_str = [severity]() {
        switch (severity) {
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return "NOTIFICATION";
            case GL_DEBUG_SEVERITY_LOW:
                return "LOW";
            case GL_DEBUG_SEVERITY_MEDIUM:
                return "MEDIUM";
            case GL_DEBUG_SEVERITY_HIGH:
                return "HIGH";
            default:
                return "???";
        }
    }();

    fprintf(
        stdout,
        "\n\nOpenGL Debug Log:\n"
        "-----------------\n"
        "Source:   %s\n"
        "Type:     %s\n"
        "Severity: %s\n"
        "Message:  '%s'\n",
        src_str,
        type_str,
        severity_str,
        message);
}

// Renderer
Renderer::Renderer(bool opengl_logging)
{
    // TODO: vs_default and vs_shadow could be merged if we used the same vertex format (which we should to save GPU
    // memory)
    Shader vs_default = CompileShader(GL_VERTEX_SHADER, VS_Common.src, VS_Common.len);
    Shader vs_shadow  = CompileShader(GL_VERTEX_SHADER, VS_ShadowVolume.src, VS_ShadowVolume.len);
    Shader fs_shadow  = CompileShader(GL_FRAGMENT_SHADER, FS_ShadowVolume.src, FS_ShadowVolume.len);

    Shader fs_ambient = CompileLightingShader(GL_FRAGMENT_SHADER, LightType::Ambient, FS_Lighting.src, FS_Lighting.len);
    Shader fs_point   = CompileLightingShader(GL_FRAGMENT_SHADER, LightType::Point, FS_Lighting.src, FS_Lighting.len);
    Shader fs_spot    = CompileLightingShader(GL_FRAGMENT_SHADER, LightType::Spot, FS_Lighting.src, FS_Lighting.len);
    Shader fs_sun     = CompileLightingShader(GL_FRAGMENT_SHADER, LightType::Sun, FS_Lighting.src, FS_Lighting.len);

    Shader gs_point
        = CompileLightingShader(GL_GEOMETRY_SHADER, LightType::Point, GS_ShadowVolume.src, GS_ShadowVolume.len);
    Shader gs_spot
        = CompileLightingShader(GL_GEOMETRY_SHADER, LightType::Spot, GS_ShadowVolume.src, GS_ShadowVolume.len);
    Shader gs_sun = CompileLightingShader(GL_GEOMETRY_SHADER, LightType::Sun, GS_ShadowVolume.src, GS_ShadowVolume.len);

    this->dl_shader[(usize)Renderer::ShaderType::Ambient] = LinkShaders(vs_default, fs_ambient);
    this->dl_shader[(usize)Renderer::ShaderType::Point]   = LinkShaders(vs_default, fs_point);
    this->dl_shader[(usize)Renderer::ShaderType::Spot]    = LinkShaders(vs_default, fs_spot);
    this->dl_shader[(usize)Renderer::ShaderType::Sun]     = LinkShaders(vs_default, fs_sun);

    // TODO: the ShaderProgram array has an extra slot, could be avoided but not a big deal
    this->sv_shader[(usize)Renderer::ShaderType::Point] = LinkShaders(vs_shadow, gs_point, fs_shadow);
    this->sv_shader[(usize)Renderer::ShaderType::Spot]  = LinkShaders(vs_shadow, gs_spot, fs_shadow);
    this->sv_shader[(usize)Renderer::ShaderType::Sun]   = LinkShaders(vs_shadow, gs_sun, fs_shadow);

    // TODO: might need different handling if more textures of each type are enabled
    for (ShaderProgram& sp : this->dl_shader) {
        sp.SetUniform("g_material.diffuse", 0);
        sp.SetUniform("g_material.specular", 1);
    }

    // TODO: synchronous is slow, not that big of an issue for now
    // TODO: this functionality doesn't exist in OpenGL 3.3
    if (opengl_logging) {
        GL(glEnable(GL_DEBUG_OUTPUT));
        GL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
        GL(glDebugMessageCallback(OpenGL_Debug_Callback, nullptr));
    }
}

void Renderer::Set_Resolution(u32 width, u32 height)
{
    GL(glViewport(0, 0, width, height));
}

void Renderer::Set_NormalMatrix(const glm::mat3& mtx_normal)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->dl_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_normal", mtx_normal);
    }

    for (ShaderProgram& sp : this->sv_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_normal", mtx_normal);
    }
}

void Renderer::Set_WorldMatrix(const glm::mat4& mtx_world)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->dl_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_world", mtx_world);
    }

    for (ShaderProgram& sp : this->sv_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_world", mtx_world);
    }
}

void Renderer::Set_ViewMatrix(const glm::mat4& mtx_view)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->dl_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_view", mtx_view);
    }

    for (ShaderProgram& sp : this->sv_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_view", mtx_view);
    }
}

void Renderer::Set_ScreenMatrix(const glm::mat4& mtx_screen)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->dl_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_screen", mtx_screen);
    }

    for (ShaderProgram& sp : this->sv_shader) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_screen", mtx_screen);
    }
}

void Renderer::Set_ViewPosition(const glm::vec3& view_pos)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->dl_shader) {
        sp.UseProgram();
        sp.SetUniform("g_view_pos", view_pos);
    }

    for (ShaderProgram& sp : this->sv_shader) {
        sp.UseProgram();
        sp.SetUniform("g_view_pos", view_pos);
    }
}

void Renderer::Enable(GLenum setting)
{
    GL(glEnable(setting));
}

void Renderer::Clear(const glm::vec3& color)
{
    // TODO: this doesn't need to be set with every clear, we could track its value
    GL(glClearColor(color.r, color.g, color.b, 1.0f));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}

void Renderer::Render_Light(const AmbientLight& light, const Object& obj)
{
    ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Ambient];
    sp.UseProgram();

    sp.SetUniform("g_light_source.color", light.color * light.intensity);

    obj.Draw(sp);
}

void Renderer::Render_Light(const PointLight& light, const Object& obj)
{
    ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Point];
    sp.UseProgram();

    sp.SetUniform("g_light_source.pos", light.pos);
    sp.SetUniform("g_light_source.color", light.color * light.intensity);

    obj.Draw(sp);
}

void Renderer::Render_Shadow(const PointLight& light, const Object& obj)
{
    ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Point];
    sp.UseProgram();

    sp.SetUniform("g_light_source.pos", light.pos);

    obj.ComputeShadow(sp);
}

void Renderer::Render_Light(const SpotLight& light, const Object& obj)
{
    ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Spot];
    sp.UseProgram();

    sp.SetUniform("g_light_source.pos", light.pos);
    sp.SetUniform("g_light_source.dir", light.dir);
    sp.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
    sp.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);
    sp.SetUniform("g_light_source.color", light.color * light.intensity);

    obj.Draw(sp);
}

void Renderer::Render_Shadow(const SpotLight& light, const Object& obj)
{
    ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Spot];
    sp.UseProgram();

    sp.SetUniform("g_light_source.pos", light.pos);
    sp.SetUniform("g_light_source.dir", light.dir);
    sp.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
    sp.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);

    obj.ComputeShadow(sp);
}

void Renderer::Render_Light(const SunLight& light, const Object& obj)
{
    ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Sun];
    sp.UseProgram();

    sp.SetUniform("g_light_source.dir", light.dir);
    sp.SetUniform("g_light_source.color", light.color * light.intensity);

    obj.Draw(sp);
}

void Renderer::Render_Shadow(const SunLight& light, const Object& obj)
{
    ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Sun];
    sp.UseProgram();

    sp.SetUniform("g_light_source.dir", light.dir);

    obj.ComputeShadow(sp);
}
