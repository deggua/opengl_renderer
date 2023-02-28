#include "opengl.hpp"

#include <GL/glew.h>
#include <stb/stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"

Handle<GLuint> ShaderProgram::Bound = 0;
Handle<GLuint> Texture2D::Bound     = 0;

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

void Texture2D::LoadTexture(FILE* fd)
{
    // TODO: some of these should be methods like tex.SetParameter, tex.GenerateMipmap, etc.
    this->Bind();

    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
    GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    int width, height, num_channels;
    u8* image_data = stbi_load_from_file(fd, &width, &height, &num_channels, 0);

    if (!image_data) {
        ABORT("Failed to load texture");
    }

    GLenum color_fmt = (num_channels == 3 ? GL_RGB : GL_RGBA);

    GL(glTexImage2D(GL_TEXTURE_2D, 0, color_fmt, width, height, 0, color_fmt, GL_UNSIGNED_BYTE, image_data));
    GL(glGenerateMipmap(GL_TEXTURE_2D));

    stbi_image_free(image_data);
}

// Texture2D
Texture2D::Texture2D(FILE* fd)
{
    GL(glGenTextures(1, &this->handle));

    this->LoadTexture(fd);
}

Texture2D::Texture2D(const char* file_path)
{
    GL(glGenTextures(1, &this->handle));

    FILE* fd = fopen(file_path, "rb");
    if (!fd) {
        ABORT("Failed to open file: %s", file_path);
    }

    this->LoadTexture(fd);

    fclose(fd);
}

// TODO: this doesn't completely eliminate needless binds because we still do binding for slots
// in the render loop, we could track what texture handles are assigned to what texture slots
// but I'm not sure if the slots are used for other things as well, e.g. non-Texture2Ds
void Texture2D::Bind() const
{
    if (Texture2D::Bound.handle == this->handle) {
        return;
    }

    GL(glBindTexture(GL_TEXTURE_2D, this->handle));
    Texture2D::Bound.handle = this->handle;
}

// Uniform
Uniform::Uniform(const char* name, GLint location)
{
    this->name   = name;
    this->handle = location;
}

// VAO
VAO::VAO()
{
    GL(glGenVertexArrays(1, &this->handle));
}

VAO::~VAO()
{
    GL(glDeleteVertexArrays(1, &this->handle));
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
VBO::VBO()
{
    GL(glGenBuffers(1, &this->handle));
}

VBO::~VBO()
{
    GL(glDeleteBuffers(1, &this->handle));
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
    this->Unbind();
}

// EBO
EBO::EBO()
{
    GL(glGenBuffers(1, &this->handle));
}

EBO::~EBO()
{
    GL(glDeleteBuffers(1, &this->handle));
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
    this->Unbind();
}
