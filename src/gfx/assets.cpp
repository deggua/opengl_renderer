#include "assets.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <string>
#include <vector>

AssetPool<Texture2D> TexturePool(32);

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
Material::Material(Texture2D* diffuse, Texture2D* specular, f32 gloss)
{
    this->diffuse  = diffuse;
    this->specular = specular;
    this->gloss    = gloss;
}

void Material::Use(ShaderProgram& sp) const
{
    GL(glActiveTexture(GL_TEXTURE0));
    this->diffuse->Bind();

    GL(glActiveTexture(GL_TEXTURE1));
    this->specular->Bind();

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

        Texture2D* diffuse_tex;
        Texture2D* specular_tex;
        f32        gloss;

        // TODO: the default textures should probably be pre-existing in the AssetManager
        if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
            aiString diffuse_path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path);
            std::string full_path = std::string(directory) + "/" + diffuse_path.C_Str();
            diffuse_tex           = TexturePool.CheckOut(full_path);
        } else {
            // TODO: we should probably insert default specular/diffuse textures into the texture pool or something
            diffuse_tex = TexturePool.CheckOut("$NO_DIFFUSE");
        }

        if (material->GetTextureCount(aiTextureType_SPECULAR)) {
            aiString specular_path;
            material->GetTexture(aiTextureType_SPECULAR, 0, &specular_path);
            std::string full_path = std::string(directory) + "/" + specular_path.C_Str();
            specular_tex          = TexturePool.CheckOut(full_path);
        } else {
            specular_tex = TexturePool.CheckOut("$NO_SPECULAR");
        }

        // set the gloss
        material->Get(AI_MATKEY_SHININESS, gloss);

        return Object{Mesh(vertices, indices), Material(diffuse_tex, specular_tex, gloss)};
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
