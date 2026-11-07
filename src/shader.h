#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>

// Loads, compiles, and links a vertex/fragment GLSL program. An active OpenGL
// context is required for construction, use, and destruction.
class Shader {
public:
    // Throws std::runtime_error with the file path or driver log if loading,
    // compilation, or linking fails.
    Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);
    ~Shader();

    // OpenGL program handles are uniquely owned and cannot be copied.
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Makes this program current for subsequent OpenGL draw calls.
    void use() const;

    // Sets a mat4 uniform on this program. Call use() first; throws if the
    // named uniform is not active in the linked program.
    void setMatrix4(const std::string& name, const glm::mat4& matrix) const;
    void setVector3(const std::string& name, const glm::vec3& vector) const;
    void setFloat(const std::string& name, float value) const;
    void setInt(const std::string& name, int value) const;

    // Returns the OpenGL program handle, for example when querying attributes.
    GLuint id() const;

private:
    GLuint program = 0;
};
