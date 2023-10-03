#include "assets.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "gfx/cache.hpp"
#include "math/math.hpp"

AssetCache<Texture2D> TexturePool(32);

template<>
struct std::hash<aiVector3D> {
    std::size_t operator()(const aiVector3D& vec) const noexcept
    {
        return std::hash<f32>{}(vec.x) ^ std::hash<f32>{}(vec.y) ^ std::hash<f32>{}(vec.z);
    }
};

struct HalfEdge {
    GLuint idx[2];

    HalfEdge()
    {
        this->idx[0] = 0;
        this->idx[1] = 0;
    }

    HalfEdge(GLuint first, GLuint second)
    {
        this->idx[0] = first;
        this->idx[1] = second;
    }

    bool operator==(const HalfEdge& rhs) const
    {
        return this->idx[0] == rhs.idx[0] && this->idx[1] == rhs.idx[1];
    }
};

template<>
struct std::hash<HalfEdge> {
    std::size_t operator()(const HalfEdge& hedge) const noexcept
    {
        return std::hash<GLuint>{}(hedge.idx[0]) ^ std::hash<GLuint>{}(hedge.idx[1]);
    }
};

struct Face {
    GLuint idx[3];

    Face()
    {
        this->idx[0] = 0;
        this->idx[1] = 0;
        this->idx[2] = 0;
    }

    Face(GLuint i0, GLuint i1, GLuint i2)
    {
        this->idx[0] = i0;
        this->idx[1] = i1;
        this->idx[2] = i2;
    }

    bool operator==(const Face& rhs) const
    {
        bool first = this->idx[0] == rhs.idx[0] && this->idx[1] == rhs.idx[1]
                     && this->idx[2] == rhs.idx[2];
        bool second = this->idx[0] == rhs.idx[1] && this->idx[1] == rhs.idx[2]
                      && this->idx[2] == rhs.idx[0];
        bool third = this->idx[0] == rhs.idx[2] && this->idx[1] == rhs.idx[0]
                     && this->idx[2] == rhs.idx[1];
        return first || second || third;
    }

    Face Mirror() const
    {
        return Face(this->idx[0], this->idx[2], this->idx[1]);
    }
};

template<>
struct std::hash<Face> {
    std::size_t operator()(const Face& face) const noexcept
    {
        return std::hash<GLuint>{}(face.idx[0]) ^ std::hash<GLuint>{}(face.idx[1])
               ^ std::hash<GLuint>{}(face.idx[2]);
    }
};

static glm::vec3 ConvertVector(const aiVector3D& vec)
{
    return {vec.x, vec.y, vec.z};
}

static GLuint
UniqueIndex(std::unordered_map<aiVector3D, GLuint>& vtx_map, const aiMesh& mesh, GLuint pot_index)
{
    aiVector3D& vtx = mesh.mVertices[pot_index];
    return vtx_map.at(vtx);
}

static std::vector<GLuint> ComputeAdjacencyIndices(const aiMesh& mesh)
{
    std::unordered_map<aiVector3D, GLuint> vtx_map      = {};
    std::unordered_set<Face>               unique_faces = {};
    std::unordered_map<HalfEdge, GLuint>   edge_map     = {};

    // first we filter out non-unique indices, this lets us map vertices to a unique index
    for (GLuint ii = 0; ii < mesh.mNumVertices; ii++) {
        const aiVector3D& vtx = mesh.mVertices[ii];
        if (vtx_map.find(vtx) == std::end(vtx_map)) {
            vtx_map[vtx] = ii;
        }
    }

    // even though we dedupe vertices, we might still add two identical faces because after we go
    // through the map two faces with separate indices might map to identical or semi-identical
    // faces, so we need to dedupe faces
    for (usize ii = 0; ii < mesh.mNumFaces; ii++) {
        ASSERT(mesh.mFaces[ii].mNumIndices == 3);
        GLuint i0 = UniqueIndex(vtx_map, mesh, mesh.mFaces[ii].mIndices[0]);
        GLuint i1 = UniqueIndex(vtx_map, mesh, mesh.mFaces[ii].mIndices[1]);
        GLuint i2 = UniqueIndex(vtx_map, mesh, mesh.mFaces[ii].mIndices[2]);

        Face face = Face(i0, i1, i2);
        // TODO: this does sort of work, but it also means that flat geometry (e.g. a cape that has
        // no volume) will not cast shadows, which sucks
        // TODO: use find?
        if (unique_faces.contains(face.Mirror())) {
            unique_faces.erase(face.Mirror());
        } else {
            unique_faces.insert(face);
        }
    }

    // now we should be able to map edges to two unique vertices (so long as the original mesh
    // doesn't have the edge case)
    for (const Face& face : unique_faces) {
        for (usize ii = 0; ii < 3; ii++) {
            GLuint i0 = face.idx[ii];
            GLuint i1 = face.idx[(ii + 1) % 3];
            GLuint i2 = face.idx[(ii + 2) % 3];
            edge_map.insert({HalfEdge(i0, i1), i2});
        }
    }

    // now we have a map of edges to their opposite vertex, construct the indices
    std::vector<GLuint> indices = {};
    for (const Face& face : unique_faces) {
        // see: https://ogldev.org/www/tutorial39/adjacencies.jpg
        GLuint adj[6] = {
            [0] = face.idx[0],
            [2] = face.idx[1],
            [4] = face.idx[2],
        };

        const auto& opp_e1 = edge_map.find({adj[2], adj[0]});
        const auto& opp_e5 = edge_map.find({adj[4], adj[2]});
        const auto& opp_e2 = edge_map.find({adj[0], adj[4]});

        if (opp_e1 == std::end(edge_map)) {
            adj[1] = adj[4];
        } else {
            adj[1] = opp_e1->second;
        }

        if (opp_e5 == std::end(edge_map)) {
            adj[3] = adj[0];
        } else {
            adj[3] = opp_e5->second;
        }

        if (opp_e2 == std::end(edge_map)) {
            adj[5] = adj[2];
        } else {
            adj[5] = opp_e2->second;
        }

        // if a triangle is fully adjacent to itself then disable shadow geometry
        bool i1_not_unique = (adj[1] == adj[0]) || (adj[1] == adj[2]) || (adj[1] == adj[4]);
        bool i3_not_unique = (adj[3] == adj[0]) || (adj[3] == adj[2]) || (adj[3] == adj[4]);
        bool i5_not_unique = (adj[5] == adj[0]) || (adj[5] == adj[2]) || (adj[5] == adj[4]);
        if (i1_not_unique && i3_not_unique && i5_not_unique) {
            // TODO: This is kind of a catch all for buggy geometry if the above didn't work, but it
            // isn't great for a few reasons:
            //  * Disables shadow geometry completely
            //  * Still creates an EBO and will lead to a draw call (not necessary)
            // we could fix this by returning an std::optional<std::vector<GLuint>>, but we need a
            // way for individual pieces of geometry to say they don't cast shadows as well as
            // object groups
            LOG_WARNING("Found single tri");
            return {};
        } else {
            indices.insert(indices.end(), std::begin(adj), std::end(adj));
        }
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
        visual_indices.insert(
            visual_indices.end(),
            face.mIndices,
            face.mIndices + face.mNumIndices);
    }

    // collect vertex positions, normals, tex coords
    std::vector<Vertex> vertices = {};
    for (size_t ii = 0; ii < mesh.mNumVertices; ii++) {
        glm::mat3 tangent_space_basis = Orthonormal_GramSchmidt(
            ConvertVector(mesh.mNormals[ii]),
            ConvertVector(mesh.mTangents[ii]),
            ConvertVector(mesh.mBitangents[ii]));

        Vertex vert = Vertex{
            ConvertVector(mesh.mVertices[ii]),
            tangent_space_basis[0],
            tangent_space_basis[1],
            tangent_space_basis[2],
            {0.0f, 0.0f},
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
    this->vao_visual.SetAttribute(2, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tangent));
    this->vao_visual.SetAttribute(3, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, bitangent));
    this->vao_visual.SetAttribute(4, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->vao_shadow.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao_shadow.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao_shadow.SetAttribute(2, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tangent));
    this->vao_shadow.SetAttribute(3, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, bitangent));
    this->vao_shadow.SetAttribute(4, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->vao_visual.Bind();
    this->ebo_visual.LoadData(
        visual_indices.size() * sizeof(GLuint),
        visual_indices.data(),
        GL_STATIC_DRAW);

    this->vao_shadow.Bind();
    this->ebo_shadow.LoadData(
        shadow_indices.size() * sizeof(GLuint),
        shadow_indices.data(),
        GL_STATIC_DRAW);

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
    this->diffuse  = TexturePool.Load(DefaultTexture_Diffuse);
    this->specular = TexturePool.Load(DefaultTexture_Specular);
    this->normal   = TexturePool.Load(DefaultTexture_Normal);
    this->gloss    = 1.0f;
}

Material::Material(const aiMaterial& material, std::string_view directory)
{
    if (material.GetTextureCount(aiTextureType_DIFFUSE)) {
        aiString diffuse_path;
        material.GetTexture(aiTextureType_DIFFUSE, 0, &diffuse_path);

        std::stringstream file_path;
        file_path << directory << "/" << diffuse_path.C_Str();

        this->diffuse = TexturePool.Load(file_path.str());
    } else {
        this->diffuse = TexturePool.Load(DefaultTexture_Diffuse);
    }

    if (material.GetTextureCount(aiTextureType_SPECULAR)) {
        aiString specular_path;
        material.GetTexture(aiTextureType_SPECULAR, 0, &specular_path);

        std::stringstream file_path;
        file_path << directory << "/" << specular_path.C_Str();

        this->specular = TexturePool.Load(file_path.str());
    } else {
        this->specular = TexturePool.Load(DefaultTexture_Specular);
    }

    // TODO: Displacement vs Normals vs Height?
    if (material.GetTextureCount(aiTextureType_DISPLACEMENT)) {
        aiString normal_path;
        material.GetTexture(aiTextureType_DISPLACEMENT, 0, &normal_path);

        std::stringstream file_path;
        file_path << directory << "/" << normal_path.C_Str();

        this->normal = TexturePool.Load(file_path.str());
    } else {
        this->normal = TexturePool.Load(DefaultTexture_Normal);
    }

    // set the gloss
    material.Get(AI_MATKEY_SHININESS, this->gloss);
    this->gloss = glm::min(this->gloss, 1.0f);
}

void Material::Use(ShaderProgram& sp) const
{
    this->diffuse->Bind(GL_TEXTURE0);
    this->specular->Bind(GL_TEXTURE1);
    this->normal->Bind(GL_TEXTURE2);

    sp.SetUniform("material.gloss", this->gloss);
}

/* --- Model --- */
Model::Model(const Geometry& geometry, const Material& material) :
    geometry(geometry), material(material)
{}

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
static Model
ProcessAssimpMesh(const aiScene& ai_scene, const aiMesh& ai_mesh, std::string_view directory)
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

    return Model(geometry, material);
}

static void ProcessAssimpNode(
    std::vector<Model>& objs,
    const aiScene&      scene,
    const aiNode&       node,
    std::string_view    directory)
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
    const aiScene*   scene = importer.ReadFile(
        fp.c_str(),
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        ABORT("Asset import failed for '%s'", fp.c_str());
    }

    std::string_view directory = file_path.substr(0, file_path.find_last_of('/'));
    ProcessAssimpNode(this->models, *scene, *scene->mRootNode, directory);

    LOG_INFO("Imported %zu models from '%s'", this->models.size(), fp.c_str());

    usize tri_count_visual = 0;
    usize tri_count_shadow = 0;
    for (const auto& iter : this->models) {
        tri_count_visual += iter.geometry.len_visual / 3;
        tri_count_shadow += iter.geometry.len_shadow / 6;
    };

    LOG_INFO("Visual Tri Count = %zu", tri_count_visual);
    LOG_INFO("Shadow Tri Count = %zu", tri_count_shadow);
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
    mtx           = glm::translate(mtx, this->pos);
    mtx           = glm::scale(mtx, this->scale);

    return mtx;
}

// TODO: having to recompute the world matrix is annoying, but probably gets optimized away if both
// are called adjacent
glm::mat3 Object::NormalMatrix() const
{
    glm::mat4 world_mtx  = this->WorldMatrix();
    glm::mat4 normal_mtx = glm::mat3(glm::transpose(glm::inverse(world_mtx)));

    return normal_mtx;
}

/* --- Sprite3D --- */
bool Sprite3D::is_vao_initialized = false;
VAO  Sprite3D::vao;
VBO  Sprite3D::vbo;

static const Vertex sprite_quad[] = {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {0.0f, 0.0f}},
    {  {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {1.0f, 1.0f}},
    { {-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {0.0f, 1.0f}},

    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {0.0f, 0.0f}},
    { {1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {1.0f, 0.0f}},
    {  {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {}, {}, {1.0f, 1.0f}},
};

Sprite3D::Sprite3D(std::string_view tex_path)
{
    this->sprite = TexturePool.Load(tex_path);

    if (this->is_vao_initialized) {
        return;
    }

    this->vao.Reserve();
    this->vbo.Reserve();

    this->vbo.LoadData(sizeof(sprite_quad), &sprite_quad, GL_STATIC_DRAW);

    this->vbo.Bind();
    this->vao.SetAttribute(0, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, pos));
    this->vao.SetAttribute(1, 3, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, norm));
    this->vao.SetAttribute(2, 2, GL_FLOAT, sizeof(Vertex), offsetof(Vertex, tex));

    this->is_vao_initialized = true;
}

glm::vec3 Sprite3D::Position() const
{
    return this->pos;
}

Sprite3D& Sprite3D::Position(const glm::vec3& pos)
{
    this->pos = pos;
    return *this;
}

glm::vec3 Sprite3D::Scale() const
{
    return this->scale;
}

Sprite3D& Sprite3D::Scale(const glm::vec3& scale)
{
    this->scale = scale;
    return *this;
}

Sprite3D& Sprite3D::Scale(f32 scale)
{
    this->scale = glm::vec3(scale);
    return *this;
}

glm::vec3 Sprite3D::Tint() const
{
    return this->tint;
}

Sprite3D& Sprite3D::Tint(f32 r, f32 g, f32 b)
{
    this->tint = glm::vec3(r, g, b);
    return *this;
}

Sprite3D& Sprite3D::Tint(const glm::vec3& color)
{
    this->tint = color;
    return *this;
}

f32 Sprite3D::Intensity() const
{
    return this->intensity;
}

Sprite3D& Sprite3D::Intensity(f32 intensity)
{
    this->intensity = intensity;
    return *this;
}

// TODO: could be useful to have a sprite that rotates in just the xz-plane
glm::mat4 Sprite3D::WorldMatrix() const
{
    return glm::translate(glm::mat4(1.0f), this->pos);
}

void Sprite3D::Draw(ShaderProgram& sp) const
{
    (void)sp;

    this->sprite->Bind(GL_TEXTURE0);

    this->vao.Bind();
    GL(glDrawArrays(GL_TRIANGLES, 0, lengthof(sprite_quad)));
    this->vao.Unbind();
}
