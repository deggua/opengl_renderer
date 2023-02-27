#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "common.hpp"

struct OpenGL_Handle {
    GLuint handle = 0;

    OpenGL_Handle() = default;

    OpenGL_Handle(GLuint hand)
    {
        this->handle = hand;
    }

    OpenGL_Handle(GLint hand)
    {
        ASSERT(hand >= 0);
        this->handle = hand;
    }

    explicit operator GLuint() const
    {
        return this->handle;
    }

    explicit operator GLint() const
    {
        GLint ret = (GLint)this->handle;
        ASSERT(ret >= 0);
        return ret;
    }
};

struct Uniform : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;
};

struct Texture2D : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;

    Texture2D(const char* file_path);
    Texture2D(FILE* fd);

    void Bind() const;

private:
    void LoadTexture(FILE* fd);
};

struct Shader : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;
};

struct ShaderProgram : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;

    void UseProgram() const
    {
        GL(glUseProgram(GLuint(*this)));
    }

    template<typename T>
    void SetUniform(const char* name, const T& value) const
    {
        // TODO: retrieving the uniform's position and changing the shader program every time is
        // ineffecient, we should cache the location and avoid calling glUseProgram here
        GLint location;
        GL(location = glGetUniformLocation(GLuint(*this), name));
        GL(glUseProgram(GLuint(*this)));

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

struct VAO : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;

    VAO();
    ~VAO();

    void Bind() const;
    void SetAttribute(GLuint index, GLint components, GLenum type, GLsizei stride, uintptr_t offset);
};

struct VBO : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;

    VBO();
    ~VBO();

    void Bind() const;
    void LoadData(size_t size, const void* data, GLenum usage) const;
};

struct EBO : OpenGL_Handle {
    using OpenGL_Handle::OpenGL_Handle;

    EBO();
    ~EBO();

    void Bind() const;
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
