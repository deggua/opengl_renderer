#include "opengl.hpp"

#include <GL/glew.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"
#include "utils/settings.hpp"

// TODO: Texture binds should take the active texture as an argument

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
    // TODO: some of these should be methods like tex.SetParameter, tex.GenerateMipmap, etc. (I
    // think, maybe they can't be changed after loading)
    this->Reserve();
    this->Bind(GL_TEXTURE0);

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    // TODO: this should be a configureable setting, also we shouldn't grab the value every time we
    // load a texture
    GLfloat max_anistropy;
    GL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_anistropy));
    GLfloat anistropy = glm::clamp((f32)settings.af_samples, 1.0f, max_anistropy);
    GL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anistropy));

    int width, height, num_channels;
    u8* image_data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
    if (!image_data) {
        ABORT("Failed to load texture");
    }

    // NOTE: this is pretty dumb, but a common convention for normal maps
    // TODO: this doesn't detect, e.g. specular maps, but we don't have any yet
    bool is_srgb = (path.find("_ddn.") == std::string::npos);

    GLenum color_fmt = (num_channels == 3) ? GL_RGB : GL_RGBA;
    GLenum internal_fmt;
    if (is_srgb) {
        internal_fmt = (num_channels == 3) ? GL_SRGB8 : GL_SRGB8_ALPHA8;
    } else {
        internal_fmt = (num_channels == 3) ? GL_RGB8 : GL_RGBA8;
    }

    GL(glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internal_fmt,
        width,
        height,
        0,
        color_fmt,
        GL_UNSIGNED_BYTE,
        image_data));
    GL(glGenerateMipmap(GL_TEXTURE_2D));

    stbi_image_free(image_data);

    this->Unbind(GL_TEXTURE0);
}

Texture2D::Texture2D(const glm::vec4& color)
{
    this->Reserve();
    this->Bind(GL_TEXTURE0);

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    const u8 buf[4]
        = {(u8)(color.r * 255.0f),
           (u8)(color.g * 255.0f),
           (u8)(color.b * 255.0f),
           (u8)(color.a * 255.0f)};

    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf));

    this->Unbind(GL_TEXTURE0);
}

void Texture2D::Bind(GLenum texture_slot) const
{
    ASSERT(this->handle != 0);

    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D, this->handle));
}

void Texture2D::Unbind(GLenum texture_slot) const
{
    ASSERT(this->handle != 0);

    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture2D::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenTextures(1, &this->handle));
}

void Texture2D::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteTextures(1, &this->handle));
    this->handle = 0;
}

/* --- TextureRT --- */
void TextureRT::Bind(GLenum texture_slot) const
{
    ASSERT(this->handle != 0);

    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D, this->handle));
}

void TextureRT::Unbind(GLenum texture_slot) const
{
    ASSERT(this->handle != 0);

    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D, 0));
}

void TextureRT::BindMS(GLenum texture_slot) const
{
    ASSERT(this->handle != 0);

    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->handle));
}

void TextureRT::UnbindMS(GLenum texture_slot) const
{
    GL(glActiveTexture(texture_slot));
    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0));
}

void TextureRT::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenTextures(1, &this->handle));
}

void TextureRT::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteTextures(1, &this->handle));
    this->handle = 0;
}

void TextureRT::Setup(GLenum format, GLsizei width, GLsizei height)
{
    ASSERT(this->handle != 0);

    this->Bind(GL_TEXTURE0);
    GL(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGB, GL_FLOAT, nullptr));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

void TextureRT::SetupMS(GLenum format, GLsizei samples, GLsizei width, GLsizei height)
{
    ASSERT(this->handle != 0);

    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->handle));
    GL(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_TRUE));
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
    ASSERT(this->handle != 0);

    GL(glBindTexture(GL_TEXTURE_CUBE_MAP, this->handle));
}

void TextureCubemap::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenTextures(1, &this->handle));
}

void TextureCubemap::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteTextures(1, &this->handle));
    this->handle = 0;
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
    ASSERT(this->handle == 0);

    GL(glGenVertexArrays(1, &this->handle));
}

void VAO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteVertexArrays(1, &this->handle));
    this->handle = 0;
}

void VAO::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindVertexArray(this->handle));
}

void VAO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindVertexArray(0));
}

// TODO: could be templated to infer the appropriate enum type
// TODO: maybe there's a better way to wrap this in general
// TODO: do we need access to the normalization argument?
void VAO::SetAttribute(
    GLuint    index,
    GLint     components,
    GLenum    type,
    GLsizei   stride,
    uintptr_t offset)
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
    ASSERT(this->handle == 0);

    GL(glGenBuffers(1, &this->handle));
}

void VBO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteBuffers(1, &this->handle));
    this->handle = 0;
}

void VBO::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_ARRAY_BUFFER, this->handle));
}

void VBO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VBO::LoadData(size_t size, const void* data, GLenum usage) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

// EBO
void EBO::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenBuffers(1, &this->handle));
}

void EBO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteBuffers(1, &this->handle));
    this->handle = 0;
}

void EBO::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->handle));
}

void EBO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void EBO::LoadData(size_t size, const void* data, GLenum usage) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, usage));
}

/* --- UBO --- */
void UBO::Reserve(size_t size)
{
    ASSERT(this->handle == 0);

    GL(glGenBuffers(1, &this->handle));
    this->Bind();
    GL(glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STATIC_DRAW));
    this->Unbind();
}

void UBO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteBuffers(1, &this->handle));
    this->handle = 0;
}

void UBO::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_UNIFORM_BUFFER, this->handle));
}

void UBO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindBuffer(GL_UNIFORM_BUFFER, 0));
}

void UBO::SubData(size_t offset, size_t size, const void* data) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data));
    this->Unbind();
}

void UBO::BindSlot(GLuint index) const
{
    ASSERT(this->handle != 0);

    GL(glBindBufferBase(GL_UNIFORM_BUFFER, index, this->handle));
}

/* --- RBO --- */
void RBO::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenRenderbuffers(1, &this->handle));
}

void RBO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteRenderbuffers(1, &this->handle));
    this->handle = 0;
}

void RBO::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindRenderbuffer(GL_RENDERBUFFER, this->handle));
}

void RBO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void RBO::CreateStorage(GLenum internal_format, GLsizei samples, GLsizei width, GLsizei height)
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internal_format, width, height));
}

/* --- FBO --- */
void FBO::Reserve()
{
    ASSERT(this->handle == 0);

    GL(glGenFramebuffers(1, &this->handle));
}

void FBO::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteFramebuffers(1, &this->handle));
    this->handle = 0;
}

void FBO::Bind() const
{
    // NOTE: the default FBO (handle = 0) is actually the default FBO object
    // ASSERT(this->handle != 0);
    GL(glBindFramebuffer(GL_FRAMEBUFFER, this->handle));
}

void FBO::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FBO::Attach(RBO rbo, GLenum attachment) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo.handle));
}

void FBO::Attach(TextureRT tex_rt, GLenum attachment) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, tex_rt.handle, 0));
}

void FBO::AttachMS(TextureRT tex_rt, GLenum attachment) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        attachment,
        GL_TEXTURE_2D_MULTISAMPLE,
        tex_rt.handle,
        0));
}

void FBO::Attach(Image2D img_rt, GLenum attachment) const
{
    ASSERT(this->handle != 0);

    this->Bind();
    GL(glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        attachment,
        GL_TEXTURE_2D_MULTISAMPLE,
        img_rt.handle,
        0));
}

void FBO::CheckComplete() const
{
    ASSERT(this->handle != 0);

    this->Bind();
    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
}

/* --- Query --- */
void Query::Reserve()
{
    ASSERT(this->handle == 0);
    GL(glGenQueries(1, &this->handle));
}

void Query::Delete()
{
    if (this->handle == 0) {
        return;
    }

    GL(glDeleteQueries(1, &this->handle));

    this->handle = 0;
}

void Query::RecordTimestamp()
{
    ASSERT(this->handle != 0);
    GL(glQueryCounter(this->handle, GL_TIMESTAMP));
}

u64 Query::RetrieveValue() const
{
    ASSERT(this->handle != 0);

    GLuint64 value;
    GL(glGetQueryObjectui64v(this->handle, GL_QUERY_RESULT, &value));

    return (u64)value;
}

/* --- Image2D --- */
void Image2D::Reserve(GLenum pix_fmt, size_t width, size_t height)
{
    ASSERT(this->handle == 0);

    GL(glGenTextures(1, &this->handle));
    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->handle));

    GL(glTexStorage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        settings.msaa_samples,
        pix_fmt,
        width,
        height,
        GL_TRUE));
    this->pix_fmt = pix_fmt;
}

void Image2D::Delete()
{
    ASSERT(this->handle != 0);

    GL(glDeleteTextures(1, &this->handle));

    this->handle = 0;
}

void Image2D::Bind() const
{
    ASSERT(this->handle != 0);

    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->handle));
}

void Image2D::Unbind() const
{
    ASSERT(this->handle != 0);

    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0));
}

void Image2D::BindImage(GLuint image_slot, GLenum access) const
{
    ASSERT(this->handle != 0);

    GL(glBindImageTexture(image_slot, this->handle, 0, GL_FALSE, 0, access, GL_R32F));
}

void Image2D::UnbindImage() const
{
    ASSERT(this->handle != 0);

    GL(glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F));
}

void Image2D::Clear()
{
    ASSERT(this->handle != 0);

    // TODO: need to handle where this isn't the case
    GL(glClearTexImage(this->handle, 0, GL_RED, GL_FLOAT, NULL));
}
