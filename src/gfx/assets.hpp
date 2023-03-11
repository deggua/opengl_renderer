#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "gfx/opengl.hpp"

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

// TODO: mesh pool
// TODO: it's a little wasteful to construct a VAO + EBO for shadows if we're not going to draw them
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

// TODO: we should have a way to cache assets in CPU memory so they're faster to load once they get unloaded
// TODO: we're storing the key string twice, which is redundant
template<class T>
struct AssetPool {
    struct Asset {
        std::string uid;
        T           value;
        u32         checkout_count = 0;
        bool        is_loaded      = false;
        bool        is_static      = false;

        template<class... Ts>
        Asset(std::string_view uid, bool is_static, Ts... args)
        {
            this->uid = uid;

            if (is_static) {
                this->value = T(args...);
            }

            this->is_static = is_static;
            this->is_loaded = is_static;
        }

        void TryLoad()
        {
            if (!is_loaded) {
                this->value     = T(this->uid.c_str());
                this->is_loaded = true;
            }

            return;
        }

        void TryUnload()
        {
            // can't unload static assets or assets currently checked out or unloaded assets
            if (this->checkout_count > 0 || this->is_static || !this->is_loaded) {
                return;
            }

            this->value.Delete();
            this->is_loaded = false;
        }

        T* Take()
        {
            this->TryLoad();
            this->checkout_count += 1;
            return &value;
        }

        static void Release(T* ref)
        {
            // TODO: we could unload when checkout_count == 0 but that's probably not ideal, better to do a bunch of
            // unloads in one pass when we need to free memory (e.g. between level loads)
            Asset* container = containerof(ref, Asset, T);
            container->checkout_count -= 1;
        }
    };

    std::unordered_map<std::string, Asset> assets = {};

    AssetPool(size_t capacity = 32)
    {
        this->assets.reserve(capacity);
    }

    template<class... Ts>
    T* Load(std::string_view uid, bool is_static, Ts... args)
    {
        // TODO: having to create a temp string is dumb, this should be avoidable
        std::string tmp  = std::string(uid);
        const auto  elem = this->assets.find(tmp);

        if (elem == std::end(this->assets)) {
            return this->assets
                .emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(tmp),
                    std::forward_as_tuple(tmp, is_static, args...))
                .first->second.Take();
        } else {
            return elem->second.Take();
        }
    }

    T* Take(std::string_view uid)
    {
        return this->Load(uid, false);
    }

    void Release(T* ref)
    {
        Asset::Release(ref);
    }

    void GarbageCollect()
    {
        for (const auto& [key, val] : this->assets) {
            val.TryUnload();
        }
    }
};

constexpr const char* DefaultTexture_Diffuse  = ".NO_DIFFUSE";
constexpr const char* DefaultTexture_Specular = ".NO_SPECULAR";
constexpr const char* DefaultTexture_Normal   = ".NO_NORMAL";

extern AssetPool<Texture2D> TexturePool;
// TODO: in order to have an 'ObjectPool' we need a separation between 'Objects' (basically mesh + texture)
// and 'GameObjects' instances of an object at particular location in the world
