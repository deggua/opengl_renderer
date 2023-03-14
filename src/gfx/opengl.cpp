#include "opengl.hpp"

#include <GL/glew.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"

Shader CompileShader(GLenum shader_type, GLsizei count, const GLchar** src, const GLint* len)
{
    ASSERT(count > 0);

    GLuint shader;
    GL(shader = glCreateShader(shader_type));
    GL(glShaderSource(shader, count, src, len));
    GL(glCompileShader(shader));

    // check vertex shader compile status
    if constexpr (RENDER_CHECK_SHADER_COMPILE) {
        GLint success;
        char  info_log[RENDER_SHADER_LOG_SIZE];
        GL(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));

        if (!success) {
            GL(glGetShaderInfoLog(shader, sizeof(info_log), nullptr, info_log));
            ABORT(
                "Compilation of shader failed:\n"
                "Reason: %s\n",
                info_log);
        }
    }

    return Shader(shader);
}

Shader CompileShader(GLenum shader_type, const char* src, i32 len)
{
    const GLchar* src_gl = src;
    const GLint   len_gl = len;
    return CompileShader(shader_type, 1, &src_gl, &len_gl);
}

/* --- Texture2D --- */
Texture2D::Texture2D(const std::string& path)
{
    // TODO: some of these should be methods like tex.SetParameter, tex.GenerateMipmap, etc. (I think, maybe they can't
    // be changed after loading)
    this->Reserve();
    this->Bind();

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // TODO: this should be a configureable setting, also we shouldn't grab the value every time we load a texture
    GLfloat max_anistropy;
    GL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anistropy));
    GLfloat anistropy = glm::clamp(16.0f, 1.0f, max_anistropy);
    GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anistropy));

    int width, height, num_channels;
    u8* image_data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
    if (!image_data) {
        ABORT("Failed to load texture");
    }

    // NOTE: this is pretty dumb, but a common convention for normal maps
    // TODO: this doesn't detect, e.g. specular maps, but we don't have any yet
    bool is_srgb = (path.find("_ddn.") == std::string::npos);
    LOG_INFO("Texture '%s' %s in sRGB", path.c_str(), is_srgb ? "is" : "is not");

    GLenum color_fmt = (num_channels == 3) ? GL_RGB : GL_RGBA;
    GLenum internal_fmt;
    if (is_srgb) {
        internal_fmt = (num_channels == 3) ? GL_SRGB8 : GL_SRGB8_ALPHA8;
    } else {
        internal_fmt = (num_channels == 3) ? GL_RGB8 : GL_RGBA8;
    }

    GL(glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt, width, height, 0, color_fmt, GL_UNSIGNED_BYTE, image_data));
    GL(glGenerateMipmap(GL_TEXTURE_2D));

    stbi_image_free(image_data);

    this->Unbind();
}

Texture2D::Texture2D(const glm::vec4& color)
{
    this->Reserve();
    this->Bind();

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    const u8 buf[4] = {(u8)(color.r * 255.0f), (u8)(color.g * 255.0f), (u8)(color.b * 255.0f), (u8)(color.a * 255.0f)};

    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf));

    this->Unbind();
}

void Texture2D::Bind() const
{
    GL(glBindTexture(GL_TEXTURE_2D, this->handle));
}

void Texture2D::Unbind() const
{
    GL(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture2D::Reserve()
{
    GL(glGenTextures(1, &this->handle));
}

void Texture2D::Delete()
{
    GL(glDeleteTextures(1, &this->handle));
}

/* --- TextureCubemap --- */
TextureCubemap::TextureCubemap(const std::array<std::string, 6>& faces)
{
    this->Reserve();
    this->Bind();

    stbi_set_flip_vertically_on_load(false);
    for (usize ii = 0; ii < faces.size(); ii++) {
        const char* path = faces[ii].c_str();

        int width, height, num_channels;
        u8* tex_data = stbi_load(path, &width, &height, &num_channels, 3);
        if (!tex_data) {
            ABORT("Failed to load cubemap texture from '%s'", path);
        }

        // NOTE: we assume cubemaps are always in sRGB space
        GL(glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + ii,
            0,
            GL_SRGB8,
            width,
            height,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            tex_data));

        stbi_image_free(tex_data);
    }
    stbi_set_flip_vertically_on_load(true);

    GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
}

void TextureCubemap::Bind() const
{
    GL(glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle));
}

void TextureCubemap::Reserve()
{
    GL(glGenTextures(1, &this->handle));
}

void TextureCubemap::Delete()
{
    GL(glDeleteTextures(1, &this->handle));
}

// Uniform
Uniform::Uniform(const char* name, GLint location)
{
    this->name   = name;
    this->handle = location;
}

// VAO
void VAO::Reserve()
{
    if (this->handle != 0) {
        return;
    }

    GL(glGenVertexArrays(1, &this->handle));
}

void VAO::Delete()
{
    if (this->handle == 0) {
        return;
    }

    GL(glDeleteVertexArrays(1, &this->handle));
    this->handle = 0;
}

void VAO::Bind() const
{
    GL(glBindVertexArray(this->handle));
}

void VAO::Unbind() const
{
    GL(glBindVertexArray(0));
}

// TODO: could be templated to infer the appropriate enum type
// TODO: maybe there's a better way to wrap this in general
// TODO: do we need access to the normalization argument?
void VAO::SetAttribute(GLuint index, GLint components, GLenum type, GLsizei stride, uintptr_t offset)
{
    ASSERT(1 <= components && components <= 4);

    this->Bind();
    GL(glVertexAttribPointer(index, components, type, GL_FALSE, stride, (void*)offset));
    GL(glEnableVertexAttribArray(index));
    this->Unbind();
}

// VBO
void VBO::Reserve()
{
    if (this->handle != 0) {
        return;
    }

    GL(glGenBuffers(1, &this->handle));
}

void VBO::Delete()
{
    if (this->handle == 0) {
        return;
    }

    GL(glDeleteBuffers(1, &this->handle));
    this->handle = 0;
}

void VBO::Bind() const
{
    GL(glBindBuffer(GL_ARRAY_BUFFER, this->handle));
}

void VBO::Unbind() const
{
    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VBO::LoadData(size_t size, const void* data, GLenum usage) const
{
    this->Bind();
    GL(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

// EBO
void EBO::Reserve()
{
    if (this->handle != 0) {
        return;
    }

    GL(glGenBuffers(1, &this->handle));
}

void EBO::Delete()
{
    if (this->handle == 0) {
        return;
    }

    GL(glDeleteBuffers(1, &this->handle));
    this->handle = 0;
}

void EBO::Bind() const
{
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->handle));
}

void EBO::Unbind() const
{
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void EBO::LoadData(size_t size, const void* data, GLenum usage) const
{
    this->Bind();
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage));
}

/* --- UBO --- */
void UBO::Reserve(size_t size)
{
    if (this->handle != 0) {
        return;
    }

    GL(glGenBuffers(1, &this->handle));
    this->Bind();
    GL(glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW));
    this->Unbind();
}

void UBO::Delete()
{
    if (this->handle == 0) {
        return;
    }

    GL(glDeleteBuffers(1, &this->handle));
}

void UBO::Bind() const
{
    GL(glBindBuffer(GL_UNIFORM_BUFFER, this->handle));
}

void UBO::Unbind() const
{
    GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UBO::SubData(size_t offset, size_t size, const void* data) const
{
    this->Bind();
    GL(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
    this->Unbind();
}

void UBO::BindSlot(GLuint index) const
{
    GL(glBindBufferBase(GL_UNIFORM_BUFFER, index, this->handle));
}
