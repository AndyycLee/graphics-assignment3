#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Unable to open shader file: " + path);
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

GLuint compileShader(GLenum type, const std::string& source, const std::string& path)
{
    const GLuint shader = glCreateShader(type);
    const char* sourceText = source.c_str();
    glShaderSource(shader, 1, &sourceText, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        // A failed shader is not attached to a program, so release it here.
        glDeleteShader(shader);
        throw std::runtime_error("Failed to compile shader '" + path + "':\n" + log);
    }

    return shader;
}
}

Shader::Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER,
                                               readFile(vertexShaderPath),
                                               vertexShaderPath);
    GLuint fragmentShader = 0;
    try {
        fragmentShader = compileShader(GL_FRAGMENT_SHADER,
                                       readFile(fragmentShaderPath),
                                       fragmentShaderPath);
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked == GL_FALSE) {
            GLint logLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
            std::string log(static_cast<std::size_t>(logLength), '\0');
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            throw std::runtime_error("Failed to link shader program:\n" + log);
        }
    } catch (...) {
        // Release every GL object created so far before forwarding the failure.
        glDeleteShader(vertexShader);
        if (fragmentShader != 0) {
            glDeleteShader(fragmentShader);
        }
        if (program != 0) {
            glDeleteProgram(program);
            program = 0;
        }
        throw;
    }

    // Once linked, the program retains the compiled shader code.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    if (program != 0) {
        glDeleteProgram(program);
    }
}

void Shader::use() const
{
    glUseProgram(program);
}

void Shader::setMatrix4(const std::string& name, const glm::mat4& matrix) const
{
    const GLint location = glGetUniformLocation(program, name.c_str());
    if (location < 0) {
        throw std::runtime_error("Shader uniform not found: " + name);
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
}

void Shader::setVector3(const std::string& name, const glm::vec3& vector) const
{
    const GLint location = glGetUniformLocation(program, name.c_str());
    if (location < 0) {
        throw std::runtime_error("Shader uniform not found: " + name);
    }
    glUniform3fv(location, 1, &vector[0]);
}

void Shader::setFloat(const std::string& name, float value) const
{
    const GLint location = glGetUniformLocation(program, name.c_str());
    if (location < 0) {
        throw std::runtime_error("Shader uniform not found: " + name);
    }
    glUniform1f(location, value);
}

void Shader::setInt(const std::string& name, int value) const
{
    const GLint location = glGetUniformLocation(program, name.c_str());
    if (location < 0) {
        throw std::runtime_error("Shader uniform not found: " + name);
    }
    glUniform1i(location, value);
}

GLuint Shader::id() const
{
    return program;
}
