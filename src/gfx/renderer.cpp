#include "renderer.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <variant>
#include <vector>

#include "common.hpp"
#include "gfx/opengl.hpp"

SHADER_FILE(VertexShader);
SHADER_FILE(FragmentShader);

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

static Shader CompileLightingShader(LightType type, const char* src, i32 len)
{
    const GLchar* source_fragments[] = {
        ShaderPreamble_Version.str,
        ShaderPreamble_Light.str,
        ShaderPreamble_LightType[(size_t)type].str,
        ShaderPreamble_Line.str,
        src,
    };

    const GLint source_lens[] = {
        (GLint)ShaderPreamble_Version.len,
        (GLint)ShaderPreamble_Light.len,
        (GLint)ShaderPreamble_LightType[(size_t)type].len,
        (GLint)ShaderPreamble_Line.len,
        (GLint)len,
    };

    return CompileShader(GL_FRAGMENT_SHADER, lengthof(source_fragments), source_fragments, source_lens);
}

// TODO: what is the point of storing/computing the linear term if it's always 1.0? Remove from table/etc
// TODO: we should handle out of range values more elegantly, there must be a formula for this based on inverse square
// law and the desired lumosity of the light or something
static DistanceFalloff ComputeFalloffParameters(f32 range)
{
    // NOTE: see http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point+Light+Attenuation
    struct FalloffEntry {
        f32       range;
        glm::vec3 k;
    };

    static constexpr FalloffEntry table[] = {
        {   .range = 7.0f,         .k = {1.0f, 0.7f, 1.8f}},
        {  .range = 13.0f,       .k = {1.0f, 0.35f, 0.44f}},
        {  .range = 20.0f,       .k = {1.0f, 0.22f, 0.20f}},
        {  .range = 32.0f,       .k = {1.0f, 0.14f, 0.07f}},
        {  .range = 50.0f,      .k = {1.0f, 0.09f, 0.032f}},
        {  .range = 65.0f,      .k = {1.0f, 0.07f, 0.017f}},
        { .range = 100.0f,    .k = {1.0f, 0.045f, 0.0075f}},
        { .range = 160.0f,    .k = {1.0f, 0.027f, 0.0028f}},
        { .range = 200.0f,    .k = {1.0f, 0.022f, 0.0019f}},
        { .range = 325.0f,    .k = {1.0f, 0.014f, 0.0007f}},
        { .range = 600.0f,    .k = {1.0f, 0.007f, 0.0002f}},
        {.range = 3250.0f, .k = {1.0f, 0.0014f, 0.000007f}},
    };

    // find the upper and lower indices bounding the range (ii, ii - 1)
    size_t ii;
    for (ii = 0; ii < lengthof(table) - 1; ii++) {
        if (table[ii].range > range) {
            break;
        }
    }

    // TODO: what about smaller distances?
    // special case for 0
    if (ii == 0) {
        return DistanceFalloff{.k = table[0].k};
    }

    // lerp the others
    f32       range_clamped = glm::clamp(range, table[ii - 1].range, table[ii].range);
    f32       blend         = (range_clamped - table[ii - 1].range) / (table[ii].range - table[ii - 1].range);
    glm::vec3 k             = glm::mix(table[ii - 1].k, table[ii].k, blend);

    return DistanceFalloff{.k = k};
}

Light CreateLight_Ambient(glm::vec3 color, f32 intensity)
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

Light CreateLight_Sun(glm::vec3 dir, glm::vec3 color, f32 intensity)
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

Light CreateLight_Spot(
    glm::vec3 pos,
    glm::vec3 dir,
    f32       range,
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
            .falloff = ComputeFalloffParameters(range),
            .inner_cutoff = glm::cos(glm::radians(inner_theta_deg)),
            .outer_cutoff = glm::cos(glm::radians(outer_theta_deg)),
            .intensity = intensity,
        },
    };

    return light;
}

Light CreateLight_Point(glm::vec3 pos, f32 range, glm::vec3 color, f32 intensity)
{
    Light light = {
        .type = LightType::Point,
        .point = {
            .pos = pos,
            .color = color,
            .falloff = ComputeFalloffParameters(range),
            .intensity = intensity,
        },
    };

    return light;
}

// Mesh
Mesh::Mesh(const std::vector<Vertex>& vertices)
{
    this->vao.Bind();

    this->vbo.Bind();
    this->vbo.LoadData(vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    this->ebo.Bind();
    // TODO: should we do anything for the EBO if we don't use it?
    // TODO: maybe we should have separate mesh types?

    this->vao.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao.SetAttribute(2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->len = vertices.size();
}

void Mesh::Draw() const
{
    this->vao.Bind();
    // TODO: needs to handle indices vs vertices, probably needs a separate type or something
    GL(glDrawArrays(GL_TRIANGLES, 0, this->len));
}

// Target Camera
glm::mat4 TargetCamera::ViewMatrix() const
{
    return glm::lookAt(this->pos, this->target, this->up);
}

// Player Camera
glm::vec3 PlayerCamera::FacingDirection() const
{
    glm::vec3 direction = {
        glm::cos(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->pitch)),
        glm::sin(glm::radians(this->yaw)) * glm::cos(glm::radians(this->pitch)),
    };
    return glm::normalize(direction);
}

glm::mat4 PlayerCamera::ViewMatrix() const
{
    return glm::lookAt(this->pos, this->pos + this->FacingDirection(), this->up);
}

// Renderer
Renderer::Renderer()
{
    Shader vs_default = CompileShader(GL_VERTEX_SHADER, VertexShader.src, VertexShader.len);

    Shader fs_ambient = CompileLightingShader(LightType::Ambient, FragmentShader.src, FragmentShader.len);
    Shader fs_point   = CompileLightingShader(LightType::Point, FragmentShader.src, FragmentShader.len);
    Shader fs_spot    = CompileLightingShader(LightType::Spot, FragmentShader.src, FragmentShader.len);
    Shader fs_sun     = CompileLightingShader(LightType::Sun, FragmentShader.src, FragmentShader.len);

    this->shaders[(size_t)Renderer::ShaderType::Ambient] = LinkShaders(vs_default, fs_ambient);
    this->shaders[(size_t)Renderer::ShaderType::Point]   = LinkShaders(vs_default, fs_point);
    this->shaders[(size_t)Renderer::ShaderType::Spot]    = LinkShaders(vs_default, fs_spot);
    this->shaders[(size_t)Renderer::ShaderType::Sun]     = LinkShaders(vs_default, fs_sun);
}

void Renderer::Set_Resolution(u32 width, u32 height)
{
    GL(glViewport(0, 0, width, height));
}

void Renderer::Set_NormalMatrix(const glm::mat3& mtx_normal)
{
    // TODO: use uniform buffer object
    for (const ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_normal", mtx_normal);
    }
}

void Renderer::Set_WorldMatrix(const glm::mat4& mtx_world)
{
    // TODO: use uniform buffer object
    for (const ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_world", mtx_world);
    }
}

void Renderer::Set_ViewMatrix(const glm::mat4& mtx_view)
{
    // TODO: use uniform buffer object
    for (const ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_view", mtx_view);
    }
}

void Renderer::Set_ScreenMatrix(const glm::mat4& mtx_screen)
{
    // TODO: use uniform buffer object
    for (const ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_screen", mtx_screen);
    }
}

void Renderer::Set_ViewPosition(const glm::vec3& view_pos)
{
    // TODO: use uniform buffer object
    for (const ShaderProgram& sp : this->shaders) {
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
    GL(glClearColor(color.r, color.g, color.b, 1.0f));
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Renderer::Render(const Mesh& mesh, const Material& mat, const Light& light)
{
    // TODO: we don't really need to send the cam_pos + view matrix every render call

    ShaderProgram sp_cur;

    // set lighting parameters
    switch (light.type) {
        case LightType::Ambient: {
            sp_cur = this->shaders[(size_t)Renderer::ShaderType::Ambient];
            sp_cur.UseProgram();

            sp_cur.SetUniform("g_light_source.color", light.ambient.color * light.ambient.intensity);
        } break;

        case LightType::Point: {
            sp_cur = this->shaders[(size_t)Renderer::ShaderType::Point];
            sp_cur.UseProgram();

            sp_cur.SetUniform("g_light_source.pos", light.point.pos);

            sp_cur.SetUniform("g_light_source.falloff.k0", light.point.falloff.k[0]);
            sp_cur.SetUniform("g_light_source.falloff.k1", light.point.falloff.k[1]);
            sp_cur.SetUniform("g_light_source.falloff.k2", light.point.falloff.k[2]);

            sp_cur.SetUniform("g_light_source.color", light.point.color);
        } break;

        case LightType::Spot: {
            sp_cur = this->shaders[(size_t)Renderer::ShaderType::Spot];
            sp_cur.UseProgram();

            sp_cur.SetUniform("g_light_source.pos", light.spot.pos);
            sp_cur.SetUniform("g_light_source.dir", light.spot.dir);

            sp_cur.SetUniform("g_light_source.inner_cutoff", light.spot.inner_cutoff);
            sp_cur.SetUniform("g_light_source.outer_cutoff", light.spot.outer_cutoff);

            sp_cur.SetUniform("g_light_source.falloff.k0", light.spot.falloff.k[0]);
            sp_cur.SetUniform("g_light_source.falloff.k1", light.spot.falloff.k[1]);
            sp_cur.SetUniform("g_light_source.falloff.k2", light.spot.falloff.k[2]);

            sp_cur.SetUniform("g_light_source.color", light.spot.color);
        } break;

        case LightType::Sun: {
            sp_cur = this->shaders[(size_t)Renderer::ShaderType::Sun];
            sp_cur.UseProgram();

            sp_cur.SetUniform("g_light_source.dir", light.sun.dir);

            sp_cur.SetUniform("g_light_source.color", light.sun.color);
        } break;
    }

    // set material parameters
    GL(glActiveTexture(GL_TEXTURE0));
    mat.diffuse.Bind();

    GL(glActiveTexture(GL_TEXTURE1));
    mat.specular.Bind();

    sp_cur.SetUniform("g_material.diffuse", 0);
    sp_cur.SetUniform("g_material.specular", 1);
    sp_cur.SetUniform("g_material.gloss", mat.gloss);

    // draw mesh data
    mesh.Draw();
}
