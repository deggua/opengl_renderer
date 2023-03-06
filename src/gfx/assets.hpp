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

    // TODO: implement
    // Material(aiMaterial& mat);
    Material(Texture2D* diffuse, Texture2D* specular, f32 gloss);

    void Use(ShaderProgram& sp) const;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
};

// TODO: mesh pool
struct Mesh {
    size_t len;

    VAO vao;
    VBO vbo;
    EBO ebo;

    // TODO: implement
    // Mesh(const aiMesh& mesh);
    Mesh(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices);

    void Draw() const;
};

struct ShadowMesh {
    size_t len;

    VAO vao;
    VBO vbo;
    EBO ebo;

    ShadowMesh(const aiMesh& mesh);

    void Draw() const;
};

struct Object {
    Mesh       mesh;
    ShadowMesh shadow_mesh;
    Material   mat;

    static std::vector<Object> LoadObjects(std::string_view file_path);

    // Object(const Mesh& mesh, const Material& mat);

    void Draw(ShaderProgram& sp) const;
    void ComputeShadow(ShaderProgram& sp) const;
};

// TODO: we should have a way to cache assets in CPU memory so they're faster to load once they get unloaded
// TODO: we're storing the key string twice, which is redundant
template<class T>
struct AssetPool {
    struct Asset {
        T           value;
        std::string uid;
        u32         checkout_count = 0;
        bool        is_loaded      = false;
        bool        is_static      = false;

        Asset(const std::string& file_path)
        {
            this->uid = file_path;
        }

        template<class... Ts>
        Asset(const std::string& uid, Ts... args)
        {
            this->uid       = uid;
            this->is_static = true;
            this->value     = T(args...);
        }

        void Load()
        {
            if (!is_loaded && !is_static) {
                this->value     = T(this->uid.c_str());
                this->is_loaded = true;
            }

            return;
        }

        void Unload()
        {
            if (this->checkout_count > 0) {
                ABORT("Can't unload an asset when checkout_count > 0");
            }

            if (this->is_static) {
                // can't unload static assets
                return;
            }

            this->value.Delete();
            this->is_loaded = false;
            checkout_count -= 1;
        }

        T* CheckOut()
        {
            this->Load();
            checkout_count += 1;
            return &value;
        }

        static void CheckIn(T* ref)
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

    // NOTE: This foricbly inserts asset into the pool, thus care should be take to make sure ids can't collide
    // i.e. use a reserved token that can't appear in a file path
    template<class... Ts>
    T* Insert(const std::string& uid, Ts... args)
    {
        if (!this->assets.contains(uid)) {
            return this->assets
                .emplace(std::piecewise_construct, std::forward_as_tuple(uid), std::forward_as_tuple(uid, args...))
                .first->second.CheckOut();
        } else {
            return this->assets.at(uid).CheckOut();
        }
    }

    template<class... Ts>
    T* Insert(const char* uid, Ts... args)
    {
        return Insert(std::string(uid), args...);
    }

    T* CheckOut(const std::string& file_path)
    {
        return this->Insert(file_path);
    }

    void CheckIn(T* ref)
    {
        Asset::CheckIn(ref);
    }
};

extern AssetPool<Texture2D> TexturePool;
