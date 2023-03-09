#include "assets.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <sstream>
#include <string>
#include <vector>

AssetPool<Texture2D> TexturePool(32);

using VertexId = u32;

struct OppositeVertexId {
    VertexId vtx_a, vtx_b;
    bool     has_b = false;

    OppositeVertexId(VertexId vtx_a)
    {
        this->vtx_a = vtx_a;
    }

    void SetVtxB(VertexId vtx)
    {
        if (this->has_b) {
            ABORT("Degenerate mesh encountered in triangle adjacency calculation");
        }

        this->vtx_b = vtx;
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
    u64 id;

    operator u64()
    {
        return id;
    }

    EdgeId(VertexId vtx_a, VertexId vtx_b)
    {
        if (vtx_a > vtx_b) {
            id = ((u64)vtx_a << 32) | ((u64)vtx_b);
        } else {
            id = ((u64)vtx_b << 32) | ((u64)vtx_a);
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
        return std::hash<u64>{}(edge.id);
    }
};

static std::vector<GLuint> ComputeAdjacencyIndices(const aiMesh& mesh)
{
    // build a map from an edge to its opposite vertices
    std::unordered_map<EdgeId, OppositeVertexId> edge_map = {};
    for (usize ii = 0; ii < mesh.mNumFaces; ii++) {
        ASSERT(mesh.mFaces[ii].mNumIndices == 3);
        for (usize jj = 0; jj < 3; jj++) {
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
    for (usize ii = 0; ii < mesh.mNumFaces; ii++) {
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

    return indices;
}

// Geometry
Geometry::Geometry(const aiMesh& mesh)
{
    // collect indices for visual component
    std::vector<GLuint> shadow_indices = ComputeAdjacencyIndices(mesh);
    std::vector<GLuint> visual_indices = {};
    for (size_t ii = 0; ii < mesh.mNumFaces; ii++) {
        aiFace face = mesh.mFaces[ii];
        visual_indices.insert(visual_indices.end(), face.mIndices, face.mIndices + face.mNumIndices);
    }

    // collect vertex positions, normals, tex coords
    std::vector<Vertex> vertices = {};
    for (size_t ii = 0; ii < mesh.mNumVertices; ii++) {
        Vertex vert = Vertex{
            {mesh.mVertices[ii].x, mesh.mVertices[ii].y, mesh.mVertices[ii].z},
            {mesh.mNormals[ii].x, mesh.mNormals[ii].y, mesh.mNormals[ii].z},
            {0.0f, 0.0f}
        };

        if (mesh.mTextureCoords[0]) {
            vert.tex.x = mesh.mTextureCoords[0][ii].x;
            vert.tex.y = mesh.mTextureCoords[0][ii].y;
        }

        vertices.push_back(vert);
    }

    this->vao_visual.Reserve();
    this->vao_shadow.Reserve();
    this->vbo.Reserve();
    this->ebo_visual.Reserve();
    this->ebo_shadow.Reserve();

    this->vbo.LoadData(vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    this->vbo.Bind();

    this->vao_visual.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao_visual.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao_visual.SetAttribute(2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->vao_shadow.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao_shadow.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao_shadow.SetAttribute(2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->vao_visual.Bind();
    this->ebo_visual.LoadData(visual_indices.size() * sizeof(GLuint), visual_indices.data(), GL_STATIC_DRAW);

    this->vao_shadow.Bind();
    this->ebo_shadow.LoadData(shadow_indices.size() * sizeof(GLuint), shadow_indices.data(), GL_STATIC_DRAW);

    this->len_visual = visual_indices.size();
    this->len_shadow = shadow_indices.size();
}

void Geometry::DrawVisual(ShaderProgram& sp) const
{
    (void)sp;

    this->vao_visual.Bind();

    // TODO: this should probably be part of VAO
    GL(glDrawElements(GL_TRIANGLES, this->len_visual, GL_UNSIGNED_INT, 0));

    this->vao_visual.Unbind();
}

void Geometry::DrawShadow(ShaderProgram& sp) const
{
    (void)sp;

    this->vao_shadow.Bind();

    GL(glDrawElements(GL_TRIANGLES_ADJACENCY, this->len_shadow, GL_UNSIGNED_INT, 0));

    this->vao_shadow.Unbind();
}

// Material
Material::Material()
{
    this->diffuse  = TexturePool.Take(DefaultTexture_Diffuse);
    this->specular = TexturePool.Take(DefaultTexture_Specular);
    this->gloss    = 0.0f;
}

Material::Material(const aiMaterial& material, std::string_view directory)
{
    if (material.GetTextureCount(aiTextureType_DIFFUSE)) {
        aiString diffuse_path;
        material.GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path);

        std::stringstream file_path;
        file_path << directory << "/" << diffuse_path.C_Str();

        this->diffuse = TexturePool.Take(file_path.str());
    } else {
        this->diffuse = TexturePool.Take(DefaultTexture_Diffuse);
    }

    if (material.GetTextureCount(aiTextureType_SPECULAR)) {
        aiString specular_path;
        material.GetTexture(aiTextureType_SPECULAR, 0, &specular_path);

        std::stringstream file_path;
        file_path << directory << "/" << specular_path.C_Str();

        this->specular = TexturePool.Take(file_path.str());
    } else {
        this->specular = TexturePool.Take(DefaultTexture_Specular);
    }

    // set the gloss
    material.Get(AI_MATKEY_SHININESS, this->gloss);
}

void Material::Use(ShaderProgram& sp) const
{
    GL(glActiveTexture(GL_TEXTURE0));
    this->diffuse->Bind();

    GL(glActiveTexture(GL_TEXTURE1));
    this->specular->Bind();

    sp.SetUniform("material.gloss", this->gloss);
}

/* --- Model --- */
Model::Model(const Geometry& geometry, const Material& material) : geometry(geometry), material(material) {}

void Model::DrawVisual(ShaderProgram& sp) const
{
    this->material.Use(sp);
    this->geometry.DrawVisual(sp);
}

void Model::DrawShadow(ShaderProgram& sp) const
{
    this->geometry.DrawShadow(sp);
}

/* --- Object --- */
static Model ProcessAssimpMesh(const aiScene& ai_scene, const aiMesh& ai_mesh, std::string_view directory)
{
    Geometry geometry = Geometry(ai_mesh);
    Material material;

    // if it has textures then use them, otherwise just use the default material
    if (ai_mesh.mMaterialIndex >= 0) {
        aiMaterial* ai_material = ai_scene.mMaterials[ai_mesh.mMaterialIndex];
        material                = Material(*ai_material, directory);
    } else {
        material = Material();
    }

    // TODO: proper constructor for object
    return Model(geometry, material);
}

static void
ProcessAssimpNode(std::vector<Model>& objs, const aiScene& scene, const aiNode& node, std::string_view directory)
{
    for (size_t ii = 0; ii < node.mNumMeshes; ii++) {
        aiMesh* mesh = scene.mMeshes[node.mMeshes[ii]];
        objs.push_back(ProcessAssimpMesh(scene, *mesh, directory));
    }

    for (size_t ii = 0; ii < node.mNumChildren; ii++) {
        ProcessAssimpNode(objs, scene, *node.mChildren[ii], directory);
    }
}

Object::Object(std::string_view file_path)
{
    std::string fp = std::string(file_path);

    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(fp.c_str(), aiProcess_Triangulate | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ABORT("Asset import failed for '%s'", fp.c_str());
    }

    std::string_view directory = file_path.substr(0, file_path.find_last_of('/'));
    ProcessAssimpNode(this->models, *scene, *scene->mRootNode, directory);

    LOG("Imported %zu models from '%s'", this->models.size(), fp.c_str());
}

void Object::DrawVisual(ShaderProgram& sp) const
{
    for (const auto& model : this->models) {
        model.DrawVisual(sp);
    }
}

void Object::DrawShadow(ShaderProgram& sp) const
{
    for (const auto& model : this->models) {
        model.DrawShadow(sp);
    }
}

glm::vec3 Object::Position() const
{
    return this->pos;
}

Object& Object::Position(const glm::vec3& new_pos)
{
    this->pos = new_pos;

    return *this;
}

glm::vec3 Object::Scale() const
{
    return this->scale;
}

Object& Object::Scale(const glm::vec3& new_scale)
{
    this->scale = new_scale;

    return *this;
}

Object& Object::Scale(f32 new_scale)
{
    this->scale = glm::vec3(new_scale);

    return *this;
}

bool Object::CastsShadows() const
{
    return this->casts_shadows;
}

Object& Object::CastsShadows(bool obj_casts_shadows)
{
    this->casts_shadows = obj_casts_shadows;

    return *this;
}

glm::mat4 Object::WorldMatrix() const
{
    glm::mat4 mtx = glm::mat4(1.0f);
    mtx           = glm::scale(mtx, this->scale);
    mtx           = glm::translate(mtx, this->pos);

    return mtx;
}

// TODO: having to recompute the world matrix is annoying, but probably gets optimized away if both are called adjacent
glm::mat3 Object::NormalMatrix() const
{
    glm::mat4 world_mtx  = this->WorldMatrix();
    glm::mat4 normal_mtx = glm::mat3(glm::transpose(glm::inverse(world_mtx)));

    return normal_mtx;
}
