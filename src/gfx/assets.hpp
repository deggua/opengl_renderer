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
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
};

// TODO: mesh pool
struct Mesh {
    usize len;

    VAO vao;
    VBO vbo;
    EBO ebo;

    Mesh(const aiMesh& mesh);

    void Draw() const;
};

// TODO: ShadowMesh and Mesh could share the vertices buffer
// TODO: we need some way to have Meshes that cast shadows and meshes that don't while exploiting the above property
struct ShadowMesh {
    usize len;

    VAO vao;
    VBO vbo;
    EBO ebo;

    ShadowMesh(const aiMesh& mesh);

    void Draw() const;
};

struct Object {
    Mesh       mesh;
    ShadowMesh shadow_mesh;
    Material   material;

    glm::vec3 position;

    Object(const Mesh& mesh, const ShadowMesh& shadow_mesh, const Material& material);

    static std::vector<Object> LoadObjects(std::string_view file_path);

    void Draw(ShaderProgram& sp) const;
    void ComputeShadow(ShaderProgram& sp) const;
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

extern AssetPool<Texture2D> TexturePool;
