#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>

#include "common.hpp"

template<typename T>
struct Handle {
    T handle = 0;

    Handle() = default;

    Handle(T hand)
    {
        this->handle = hand;
    }

    explicit operator T() const
    {
        return this->handle;
    }
};

struct Uniform : Handle<GLint> {
    using Handle<GLint>::Handle;
    std::string name;

    Uniform(const char* name, GLint location);
};

struct Texture2D : Handle<GLuint> {
    using Handle<GLuint>::Handle;

    static Handle<GLuint> Bound;

    Texture2D(const char* file_path);
    Texture2D(FILE* fd);
    Texture2D(const glm::vec4& color);

    void Bind() const;
    void Reserve();
    void Delete();

private:
    void LoadTexture(FILE* fd);
};

struct Shader : Handle<GLuint> {
    using Handle<GLuint>::Handle;
};

struct ShaderProgram : Handle<GLuint> {
    using Handle<GLuint>::Handle;

    static Handle<GLuint> Bound;

    std::vector<Uniform> uniforms;

    void UseProgram() const
    {
        if (Bound.handle == this->handle) {
            return;
        }

        GL(glUseProgram(GLuint(*this)));
        ShaderProgram::Bound = this->handle;
    }

    // TODO:
#if 0
    template<typename T>
    T GetUniform(const char* name)
    {

    }
#endif

    template<typename T>
    void SetUniform(const char* name, const T& value)
    {
        bool  location_known = false;
        GLint location;

        // see if it's in the vector of known uniforms
        for (const auto& iter : this->uniforms) {
            if (iter.name == name) {
                location       = iter.handle;
                location_known = true;
                break;
            }
        }

        // if the location is unknown add the mapping to the vector
        if (!location_known) {
            GL(location = glGetUniformLocation(GLuint(*this), name));
            if (location < 0) {
                // TODO: this seems to happen even for valid uniforms, might need to look into this
                return;
            }

            this->uniforms.emplace_back(Uniform(name, location));
        }

        this->UseProgram();

        if constexpr (std::is_same_v<T, bool>) {
            GL(glUniform1i(location, (GLint)value));
        } else if constexpr (std::is_same_v<T, GLint>) {
            GL(glUniform1i(location, value));
        } else if constexpr (std::is_same_v<T, GLuint>) {
            GL(glUniform1ui(location, value));
        } else if constexpr (std::is_same_v<T, f32>) {
            GL(glUniform1f(location, value));
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            GL(glUniform2f(location, value.x, value.y));
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            GL(glUniform3f(location, value.x, value.y, value.z));
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            GL(glUniform4f(location, value.x, value.y, value.z, value.w));
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            GL(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value)));
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            GL(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value)));
        } else {
            // TODO: is there a better way to do this?
            static_assert(!std::is_same_v<T, T>, "No associated OpenGL function for templated type T");
        }
    }
};

struct VAO : Handle<GLuint> {
    using Handle<GLuint>::Handle;

    void Reserve();
    void Delete();
    void Bind() const;
    void Unbind() const;
    void SetAttribute(GLuint index, GLint components, GLenum type, GLsizei stride, uintptr_t offset);
};

struct VBO : Handle<GLuint> {
    using Handle<GLuint>::Handle;

    void Reserve();
    void Delete();
    void Bind() const;
    void Unbind() const;
    void LoadData(size_t size, const void* data, GLenum usage) const;
};

struct EBO : Handle<GLuint> {
    using Handle<GLuint>::Handle;

    void Reserve();
    void Delete();
    void Bind() const;
    void Unbind() const;
    void LoadData(size_t size, const void* data, GLenum usage) const;
};

Shader CompileShader(GLenum shader_type, GLsizei count, const GLchar** src, const GLint* len);
Shader CompileShader(GLenum shader_type, const char* src, i32 len);

template<class... Ts, class = std::enable_if_t<std::conjunction_v<std::is_same<Shader, Ts>...>>>
void AttachShaders(ShaderProgram program, Shader shader, Ts... shaders)
{
    GL(glAttachShader((GLuint)program, (GLuint)shader));
    if constexpr (sizeof...(shaders) > 0) {
        AttachShaders(program, shaders...);
    }
}

template<class... Ts, class = std::enable_if_t<std::conjunction_v<std::is_same<Shader, Ts>...>>>
ShaderProgram LinkShaders(Shader shader, Ts... shaders)
{
    // create program
    GLuint shader_program;
    GL(shader_program = glCreateProgram());

    // attach shaders to the program
    AttachShaders(shader_program, shader, shaders...);

    // link shaders
    GL(glLinkProgram(shader_program));

    // check linker result
    if constexpr (RENDER_CHECK_SHADER_COMPILE) {
        GLint success;
        char  info_log[RENDER_SHADER_LOG_SIZE];
        GL(glGetProgramiv(shader_program, GL_LINK_STATUS, &success));

        if (!success) {
            GL(glGetProgramInfoLog(shader_program, sizeof(info_log), nullptr, info_log));
            ABORT(
                "Linking of shader program failed:\n"
                "Reason: %s\n",
                info_log);
        }
    }

    return ShaderProgram(shader_program);
}
