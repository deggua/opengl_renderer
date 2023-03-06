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

using VertexId = uint32_t;

struct OppositeVertexId {
    VertexId vtx_a, vtx_b;
    bool     has_b = false;

    OppositeVertexId(VertexId vtx_a)
    {
        this->vtx_a = vtx_a;
    }

    void SetVtxB(VertexId vtx_b)
    {
        this->vtx_b = vtx_b;
        this->has_b = true;
    }

    std::optional<VertexId> GetOpposite(VertexId vtx)
    {
        if (this->vtx_a != vtx) {
            return this->vtx_a;
        } else if (this->has_b) {
            return this->vtx_b;
        } else {
            return std::nullopt;
        }
    }
};

struct EdgeId {
    uint64_t id;

    operator uint64_t()
    {
        return id;
    }

    EdgeId(VertexId vtx_a, VertexId vtx_b)
    {
        if (vtx_a > vtx_b) {
            id = ((uint64_t)vtx_a << 32) | ((uint64_t)vtx_b);
        } else {
            id = ((uint64_t)vtx_b << 32) | ((uint64_t)vtx_a);
        }
    }
};

bool operator==(const EdgeId& lhs, const EdgeId& rhs)
{
    return (lhs.id == rhs.id);
}

template<>
struct std::hash<EdgeId> {
    std::size_t operator()(const EdgeId& edge) const noexcept
    {
        return std::hash<uint64_t>{}(edge.id);
    }
};

// TODO: certain types of geometry won't work properly with this and we don't detect it
// example: if an edge has more than 2 opposite vertices (like a multi-planar grass mesh where 1 edge shares 3+ tris)
ShadowMesh::ShadowMesh(const aiMesh& mesh)
{
    // build a map from an edge to its opposite vertices
    std::unordered_map<EdgeId, OppositeVertexId> edge_map = {};
    for (size_t ii = 0; ii < mesh.mNumFaces; ii++) {
        ASSERT(mesh.mFaces[ii].mNumIndices == 3);
        for (size_t jj = 0; jj < 3; jj++) {
            EdgeId   edge    = EdgeId(mesh.mFaces[ii].mIndices[jj], mesh.mFaces[ii].mIndices[(jj + 1) % 3]);
            VertexId vtx_opp = mesh.mFaces[ii].mIndices[(jj + 2) % 3];

            // if the edge is not in the map, then insert it with the first opp vertex as the current opp vertex
            // if the edge is in the map, then set the 2nd opp vertex to the current edge's opposite vertex
            const auto& edge_in_map = edge_map.find(edge);
            if (edge_in_map != edge_map.end()) {
                edge_in_map->second.SetVtxB(vtx_opp);
            } else {
                edge_map.insert({edge, OppositeVertexId(vtx_opp)});
            }
        }
    }

    // fill the indices array in groups of 6 indices of triangle adjacency data
    std::vector<GLuint> indices = {};
    for (size_t ii = 0; ii < mesh.mNumFaces; ii++) {
        // see: https://ogldev.org/www/tutorial39/adjacencies.jpg
        GLuint adj_indices[6] = {
            [0] = mesh.mFaces[ii].mIndices[0],
            [2] = mesh.mFaces[ii].mIndices[1],
            [4] = mesh.mFaces[ii].mIndices[2],
        };

        EdgeId   e1        = EdgeId(mesh.mFaces[ii].mIndices[0], mesh.mFaces[ii].mIndices[1]);
        VertexId e1_opp_in = mesh.mFaces[ii].mIndices[2];

        EdgeId   e2        = EdgeId(mesh.mFaces[ii].mIndices[1], mesh.mFaces[ii].mIndices[2]);
        VertexId e2_opp_in = mesh.mFaces[ii].mIndices[0];

        EdgeId   e3        = EdgeId(mesh.mFaces[ii].mIndices[2], mesh.mFaces[ii].mIndices[0]);
        VertexId e3_opp_in = mesh.mFaces[ii].mIndices[1];

        // TODO: not sure if this is exactly what we want, but the idea is that it looks folded in on itself
        // so that the edge forms a silhoutte edge in the degenerate case where the mesh is open
        adj_indices[1] = edge_map.at(e1).GetOpposite(e1_opp_in).value_or(e1_opp_in);
        adj_indices[3] = edge_map.at(e2).GetOpposite(e2_opp_in).value_or(e2_opp_in);
        adj_indices[5] = edge_map.at(e3).GetOpposite(e3_opp_in).value_or(e3_opp_in);

        indices.insert(indices.end(), std::begin(adj_indices), std::end(adj_indices));
    }

    // fill the vertex array with raw vertex positions
    std::vector<glm::vec3> vertices = {};
    for (size_t ii = 0; ii < mesh.mNumVertices; ii++) {
        vertices.push_back(glm::vec3(mesh.mVertices[ii].x, mesh.mVertices[ii].y, mesh.mVertices[ii].z));
    }

    // now finally we can setup the GPU buffers
    this->len = indices.size();

    this->vbo.Reserve();
    this->vao.Reserve();
    this->ebo.Reserve();

    this->vbo.LoadData(vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

    this->vbo.Bind();
    this->vao.SetAttribute(0, 3, GL_FLOAT, sizeof(glm::vec3), 0);

    this->vao.Bind();
    this->ebo.LoadData(indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    this->vao.Unbind();
}

void ShadowMesh::Draw() const
{
    this->vao.Bind();

    GL(glDrawElements(GL_TRIANGLES_ADJACENCY, this->len, GL_UNSIGNED_INT, 0));

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

        return Object{Mesh(vertices, indices), ShadowMesh(*mesh), Material(diffuse_tex, specular_tex, gloss)};
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

void Object::ComputeShadow(ShaderProgram& sp) const
{
    (void)sp;
    this->shadow_mesh.Draw();
}
