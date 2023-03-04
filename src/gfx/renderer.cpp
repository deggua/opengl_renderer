#include "renderer.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
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

// Mesh
Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
{
    this->len = indices.size();

    this->vbo.Reserve();
    this->vao.Reserve();
    this->ebo.Reserve();

    this->vbo.LoadData(vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    this->vbo.Bind();
    this->vao.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao.SetAttribute(2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->vao.Bind();
    this->ebo.LoadData(indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->vao.Unbind();
}

void Mesh::Draw() const
{
    this->vao.Bind();

    GL(glDrawElements(GL_TRIANGLES, this->len, GL_UNSIGNED_INT, 0));

    this->vao.Unbind();
}

// Material
Material::Material(const Texture2D& diffuse, const Texture2D& specular, f32 gloss)
{
    this->diffuse  = diffuse;
    this->specular = specular;
    this->gloss    = gloss;
}

void Material::Use(ShaderProgram& sp) const
{
    GL(glActiveTexture(GL_TEXTURE0));
    this->diffuse.Bind();

    GL(glActiveTexture(GL_TEXTURE1));
    this->specular.Bind();

    sp.SetUniform("material.gloss", this->gloss);
}

// TODO: our model of meshes, textures, and objects doesn't work very well
// we're doing a lot of copying, we don't handle multiple textures per object nicely, etc
// this looks like a real pain
static Object ProcessAssimpMesh(const aiScene* scene, const aiMesh* mesh, std::string_view directory)
{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    for (size_t ii = 0; ii < mesh->mNumVertices; ii++) {
        Vertex vert = Vertex{
            {mesh->mVertices[ii].x, mesh->mVertices[ii].y, mesh->mVertices[ii].z},
            {mesh->mNormals[ii].x, mesh->mNormals[ii].y, mesh->mNormals[ii].z},
            {0.0f, 0.0f}
        };

        if (mesh->mTextureCoords[0]) {
            vert.tex.x = mesh->mTextureCoords[0][ii].x;
            vert.tex.y = mesh->mTextureCoords[0][ii].y;
        }

        vertices.push_back(vert);
    }

    for (size_t ii = 0; ii < mesh->mNumFaces; ii++) {
        aiFace face = mesh->mFaces[ii];
        for (size_t jj = 0; jj < face.mNumIndices; jj++) {
            indices.push_back(face.mIndices[jj]);
        }
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        Texture2D   diffuse_tex, specular_tex;

        // TODO: the default textures should probably be pre-existing in the AssetManager
        if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString diffuse_path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path);
            std::string full_path = std::string(directory) + "/" + diffuse_path.C_Str();
            diffuse_tex           = Texture2D(full_path.c_str());
        } else {
            diffuse_tex = Texture2D(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        }

        if (material->GetTextureCount(aiTextureType_SPECULAR)) {
            aiString specular_path;
            material->GetTexture(aiTextureType_SPECULAR, 0, &specular_path);
            std::string full_path = std::string(directory) + "/" + specular_path.C_Str();
            specular_tex          = Texture2D(full_path.c_str());
        } else {
            specular_tex = Texture2D(glm::vec4(0, 0, 0, 0));
        }

        return Object{Mesh(vertices, indices), Material(diffuse_tex, specular_tex, 32.0f)};
    }

    ABORT("Unhandled assimp loading path");
}

static void ProcessAssimpNode(const aiScene* scene, std::vector<Object>& objs, aiNode* node, std::string_view directory)
{
    for (size_t ii = 0; ii < node->mNumMeshes; ii++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[ii]];
        objs.push_back(ProcessAssimpMesh(scene, mesh, directory));
    }

    for (size_t ii = 0; ii < node->mNumChildren; ii++) {
        ProcessAssimpNode(scene, objs, node->mChildren[ii], directory);
    }
}

// Object
std::vector<Object> Object::LoadObjects(std::string_view file_path)
{
    Assimp::Importer importer;
    const aiScene*   scene
        = importer.ReadFile(std::string(file_path).c_str(), aiProcess_Triangulate | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ABORT("Asset Import failed for '%s'", std::string(file_path).c_str());
    }

    std::vector<Object> objs;
    std::string_view    directory = file_path.substr(0, file_path.find_last_of('/'));
    ProcessAssimpNode(scene, objs, scene->mRootNode, directory);

    return objs;
}

void Object::Draw(ShaderProgram& sp) const
{
    this->mat.Use(sp);
    this->mesh.Draw();
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

    // TODO: might need different handling if more textures of each type are enabled
    for (ShaderProgram& sp : this->shaders) {
        sp.SetUniform("g_material.diffuse", 0);
        sp.SetUniform("g_material.specular", 1);
    }
}

void Renderer::Set_Resolution(u32 width, u32 height)
{
    GL(glViewport(0, 0, width, height));
}

void Renderer::Set_NormalMatrix(const glm::mat3& mtx_normal)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_normal", mtx_normal);
    }
}

void Renderer::Set_WorldMatrix(const glm::mat4& mtx_world)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_world", mtx_world);
    }
}

void Renderer::Set_ViewMatrix(const glm::mat4& mtx_view)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_view", mtx_view);
    }
}

void Renderer::Set_ScreenMatrix(const glm::mat4& mtx_screen)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->shaders) {
        sp.UseProgram();
        sp.SetUniform("g_mtx_screen", mtx_screen);
    }
}

void Renderer::Set_ViewPosition(const glm::vec3& view_pos)
{
    // TODO: use uniform buffer object
    for (ShaderProgram& sp : this->shaders) {
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
    GL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

// TODO: we should store the current program in the renderer
void Renderer::Render(const Object& obj, const Light& light)
{
    // TODO: we don't really need to send the cam_pos + view matrix every render call

    ShaderProgram* sp_cur = nullptr;

    // set lighting parameters
    switch (light.type) {
        case LightType::Ambient: {
            sp_cur = &this->shaders[(size_t)Renderer::ShaderType::Ambient];
            sp_cur->UseProgram();

            sp_cur->SetUniform("g_light_source.color", light.ambient.color * light.ambient.intensity);
        } break;

        case LightType::Point: {
            sp_cur = &this->shaders[(size_t)Renderer::ShaderType::Point];
            sp_cur->UseProgram();

            sp_cur->SetUniform("g_light_source.pos", light.point.pos);

            sp_cur->SetUniform("g_light_source.color", light.point.color * light.point.intensity);
        } break;

        case LightType::Spot: {
            sp_cur = &this->shaders[(size_t)Renderer::ShaderType::Spot];
            sp_cur->UseProgram();

            sp_cur->SetUniform("g_light_source.pos", light.spot.pos);
            sp_cur->SetUniform("g_light_source.dir", light.spot.dir);

            sp_cur->SetUniform("g_light_source.inner_cutoff", light.spot.inner_cutoff);
            sp_cur->SetUniform("g_light_source.outer_cutoff", light.spot.outer_cutoff);

            sp_cur->SetUniform("g_light_source.color", light.spot.color * light.spot.intensity);
        } break;

        case LightType::Sun: {
            sp_cur = &this->shaders[(size_t)Renderer::ShaderType::Sun];
            sp_cur->UseProgram();

            sp_cur->SetUniform("g_light_source.dir", light.sun.dir);

            sp_cur->SetUniform("g_light_source.color", light.sun.color * light.sun.intensity);
        } break;

        default: {
            ABORT("Bad shader type");
        } break;
    }

    obj.Draw(*sp_cur);
}
