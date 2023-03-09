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

    this->sv_shader[(usize)Renderer::ShaderType::Point] = LinkShaders(vs_shadow, gs_point, fs_shadow);
    this->sv_shader[(usize)Renderer::ShaderType::Spot]  = LinkShaders(vs_shadow, gs_spot, fs_shadow);
    this->sv_shader[(usize)Renderer::ShaderType::Sun]   = LinkShaders(vs_shadow, gs_sun, fs_shadow);

    // TODO: might need different handling if more textures of each type are enabled
    // TODO: using a UBO might be better but we only pay this cost once at startup
    // setup texture indices in shaders that use them
    for (ShaderProgram& sp : this->dl_shader) {
        sp.SetUniform("g_material.diffuse", 0);
        sp.SetUniform("g_material.specular", 1);
    }

    // setup the shared UBO
    this->shared_data.Reserve(SHARED_DATA_SIZE);
    this->shared_data.BindSlot(SHARED_DATA_SLOT);

    // TODO: synchronous is slow, not that big of an issue for now
    // TODO: this functionality doesn't exist in OpenGL 3.3
    if (opengl_logging) {
        GL(glEnable(GL_DEBUG_OUTPUT));
        GL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
        GL(glDebugMessageCallback(OpenGL_Debug_Callback, nullptr));
    }
}

Renderer& Renderer::Resolution(u32 width, u32 height)
{
    if (res_width == width && res_height == height) {
        return *this;
    }

    this->res_width  = width;
    this->res_height = height;
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
    this->pos_view = pos;
    return *this;
}

Renderer& Renderer::ViewMatrix(const glm::mat4& mtx)
{
    this->mtx_view = mtx;
    return *this;
}

Renderer& Renderer::ClearColor(f32 red, f32 green, f32 blue)
{
    GL(glClearColor(red, green, blue, 1.0f));
    return *this;
}

void Renderer::RenderPrepass()
{
    // clear the screen color
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

    // TODO: use UBO
    // update 'global' uniforms (ones that only change per frame basically)
    glm::mat4 mtx_proj
        = glm::perspective(glm::radians(this->fov), (f32)res_width / (f32)res_height, CLIP_NEAR, CLIP_FAR);
    this->mtx_vp = mtx_proj * mtx_view;

    // Set mtx_vp in all shader programs
    // Set view_pos in all shader programs
    SharedData tmp = {
        this->mtx_vp,
        this->pos_view,
    };
    this->shared_data.SubData(0, SHARED_DATA_SIZE, &tmp);
}

void Renderer::RenderLighting(const AmbientLight& light, const std::vector<Object>& objs)
{
    // depth
    GL(glEnable(GL_DEPTH_TEST));
    GL(glDisable(GL_DEPTH_CLAMP));
    GL(glDepthMask(GL_TRUE));
    GL(glDepthFunc(GL_LESS));

    // culling
    GL(glEnable(GL_CULL_FACE));
    GL(glCullFace(GL_BACK));
    GL(glFrontFace(GL_CCW));

    // pixel buffer
    GL(glEnable(GL_BLEND));
    GL(glBlendEquation(GL_FUNC_ADD));
    GL(glBlendFunc(GL_ONE, GL_ZERO));
    GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    // stencil buffer
    GL(glDisable(GL_STENCIL_TEST));

    ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Ambient];
    sp.UseProgram();
    sp.SetUniform("g_light_source.color", light.color * light.intensity);

    for (const auto& obj : objs) {
        // object parameters
        sp.SetUniform("g_mtx_world", obj.WorldMatrix());
        sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
        sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

        obj.DrawVisual(sp);
    }
}

void Renderer::RenderLighting(const PointLight& light, const std::vector<Object>& objs)
{
    /* Stencil Shadow Volume Pass*/
    {
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

        ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Point];
        sp.UseProgram();
        sp.SetUniform("g_light_source.pos", light.pos);

        for (const auto& obj : objs) {
            if (obj.CastsShadows()) {
                sp.SetUniform("g_mtx_world", obj.WorldMatrix());
                sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
                sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

                obj.DrawShadow(sp);
            }
        }
    }

    /* Pixel Lighting Calculation Pass */
    {
        // depth
        GL(glEnable(GL_DEPTH_TEST));
        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glDepthMask(GL_TRUE));
        GL(glDepthFunc(GL_LEQUAL));

        // culling
        GL(glEnable(GL_CULL_FACE));

        // pixel buffer
        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glBlendFunc(GL_ONE, GL_ONE));

        // stencil buffer
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));

        ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Point];
        sp.UseProgram();
        sp.SetUniform("g_light_source.pos", light.pos);
        sp.SetUniform("g_light_source.color", light.color * light.intensity);

        for (const auto& obj : objs) {
            // object parameters
            sp.SetUniform("g_mtx_world", obj.WorldMatrix());
            sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
            sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

            obj.DrawVisual(sp);
        }
    }

    // clear the stencil buffer once we're done
    GL(glClear(GL_STENCIL_BUFFER_BIT));
}

void Renderer::RenderLighting(const SpotLight& light, const std::vector<Object>& objs)
{
    /* Stencil Shadow Volume Pass*/
    {
        // depth
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

        ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Spot];
        sp.UseProgram();
        sp.SetUniform("g_light_source.pos", light.pos);
        sp.SetUniform("g_light_source.dir", light.dir);
        sp.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
        sp.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);

        for (const auto& obj : objs) {
            if (obj.CastsShadows()) {
                sp.SetUniform("g_mtx_world", obj.WorldMatrix());
                sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
                sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

                obj.DrawShadow(sp);
            }
        }
    }

    /* Pixel Lighting Calculation Pass */
    {
        // depth
        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glDepthMask(GL_TRUE));
        GL(glDepthFunc(GL_LEQUAL));

        // culling
        GL(glEnable(GL_CULL_FACE));

        // pixel buffer
        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glBlendFunc(GL_ONE, GL_ONE));

        // stencil buffer
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));

        ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Spot];
        sp.UseProgram();
        sp.SetUniform("g_light_source.pos", light.pos);
        sp.SetUniform("g_light_source.dir", light.dir);
        sp.SetUniform("g_light_source.inner_cutoff", light.inner_cutoff);
        sp.SetUniform("g_light_source.outer_cutoff", light.outer_cutoff);
        sp.SetUniform("g_light_source.color", light.color * light.intensity);

        for (const auto& obj : objs) {
            sp.SetUniform("g_mtx_world", obj.WorldMatrix());
            sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
            sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

            obj.DrawVisual(sp);
        }
    }

    // clear the stencil buffer once we're done
    GL(glClear(GL_STENCIL_BUFFER_BIT));
}

void Renderer::RenderLighting(const SunLight& light, const std::vector<Object>& objs)
{
    /* Stencil Shadow Volume Pass*/
    {
        // depth
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

        ShaderProgram& sp = this->sv_shader[(usize)Renderer::ShaderType::Sun];
        sp.UseProgram();
        sp.SetUniform("g_light_source.dir", light.dir);

        for (const auto& obj : objs) {
            if (obj.CastsShadows()) {
                sp.SetUniform("g_mtx_world", obj.WorldMatrix());
                sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
                sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

                obj.DrawShadow(sp);
            }
        }
    }

    /* Pixel Lighting Calculation Pass */
    {
        // depth
        GL(glDisable(GL_DEPTH_CLAMP));
        GL(glDepthMask(GL_TRUE));
        GL(glDepthFunc(GL_LEQUAL));

        // culling
        GL(glEnable(GL_CULL_FACE));

        // pixel buffer
        GL(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        GL(glBlendFunc(GL_ONE, GL_ONE));

        // stencil buffer
        GL(glStencilFunc(GL_EQUAL, 0x0, 0xFF));
        GL(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));

        ShaderProgram& sp = this->dl_shader[(usize)Renderer::ShaderType::Sun];
        sp.UseProgram();
        sp.SetUniform("g_light_source.dir", light.dir);
        sp.SetUniform("g_light_source.color", light.color * light.intensity);

        for (const auto& obj : objs) {
            sp.SetUniform("g_mtx_world", obj.WorldMatrix());
            sp.SetUniform("g_mtx_normal", obj.NormalMatrix());
            sp.SetUniform("g_mtx_wvp", this->mtx_vp * obj.WorldMatrix());

            obj.DrawVisual(sp);
        }
    }

    // clear the stencil buffer once we're done
    GL(glClear(GL_STENCIL_BUFFER_BIT));
}
