#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "gfx/cache.hpp"
#include "gfx/opengl.hpp"

constexpr const char* DefaultTexture_Diffuse  = ".NO_DIFFUSE";
constexpr const char* DefaultTexture_Specular = ".NO_SPECULAR";
constexpr const char* DefaultTexture_Normal   = ".NO_NORMAL";

extern AssetCache<Texture2D> TexturePool;

// TODO: in order to have an 'ObjectPool' we need a separation between 'Objects' (basically mesh + texture)
// and 'GameObjects' instances of an object at particular location in the world

struct Material {
    Texture2D* diffuse;
    Texture2D* specular;
    f32        gloss;

    Material();
    Material(const aiMaterial& material, std::string_view directory);

    void Use(ShaderProgram& sp) const;
};

struct Vertex {
    glm::vec3 pos  = {0.0f, 0.0f, 0.0f};
    glm::vec3 norm = {0.0f, 0.0f, 0.0f};
    glm::vec2 tex  = {0.0f, 0.0f};
};

struct Geometry {
    usize len_visual;
    usize len_shadow;

    VAO vao_visual;
    VAO vao_shadow;
    VBO vbo;
    EBO ebo_visual;
    EBO ebo_shadow;

    Geometry(const aiMesh& mesh);

    void DrawVisual(ShaderProgram& sp) const;
    void DrawShadow(ShaderProgram& sp) const;
};

// TODO: Model cache
struct Model {
    Geometry geometry;
    Material material;

    Model(const Geometry& geometry, const Material& material);

    void DrawVisual(ShaderProgram& sp) const;
    void DrawShadow(ShaderProgram& sp) const;
};

// TODO: I don't like that we have to separately expose draw shadows and draw visual
// but the way shaders work doesn't facilitate a better interface in the current arch
// TODO: we could avoid having to recompute the matrices if we cached the world/normal matrices
struct Object {
    std::vector<Model> models;

    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    glm::vec3 pos   = {0.0f, 0.0f, 0.0f};
    // TODO: rotation

    bool casts_shadows = false;

    Object(std::string_view file_path);

    void DrawVisual(ShaderProgram& sp) const;
    void DrawShadow(ShaderProgram& sp) const;

    glm::vec3 Position() const;
    Object&   Position(const glm::vec3& new_pos);

    glm::vec3 Scale() const;
    Object&   Scale(const glm::vec3& new_scale);
    Object&   Scale(f32 new_scale);

    bool    CastsShadows() const;
    Object& CastsShadows(bool casts_shadows);

    glm::mat4 WorldMatrix() const;
    glm::mat3 NormalMatrix() const;
};
