#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "shader.h"

#include <string>
#include <vector>

struct Vertex {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 0.0f, 1.0f};
    glm::vec2 texCoord{0.0f};
    glm::vec4 color{1.0f};
};

struct Material {
    glm::vec3 ambient{0.2f};
    glm::vec3 diffuse{0.8f};
    glm::vec3 specular{0.2f};
    float shininess = 32.0f;
    bool lightingEnabled = true;
};

// An indexed 3D mesh rendered with OpenGL. Create, draw, and destroy it while
// its OpenGL context is current on the calling thread.
class Mesh {
public:
    // Uploads vertex attributes and triangle indices, loads its shaders, and
    // optionally loads a diffuse texture image. Each index triplet selects a
    // triangle; every index must refer to a vertex.
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices,
         const std::string& vertexShaderPath,
         const std::string& fragmentShaderPath,
         const std::string& texturePath = {},
         const Material& material = {});

    // Position-only convenience overload. Vertices use a +Z normal, no UVs,
    // and a gray color.
    Mesh(const std::vector<glm::vec3>& vertices,
         const std::vector<unsigned int>& indices,
         const std::string& vertexShaderPath,
         const std::string& fragmentShaderPath,
         const std::string& texturePath = {},
         const Material& material = {});
    ~Mesh();

    // Mesh owns OpenGL buffer handles, so it cannot be copied.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Applies an affine transform in mesh-local coordinates. Repeated transforms
    // are post-multiplied onto the current model transform.
    void applyTransform(const glm::mat4& transform);

    // Draws the indexed triangles using the supplied view and projection matrices.
    // The shader must provide the attributes and uniforms used by mesh.vert/frag.
    void draw(const glm::mat4& view, const glm::mat4& projection) const;

private:
    Shader shader;
    Material material;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLuint indexBuffer = 0;
    GLuint texture = 0;
    GLsizei indexCount = 0;
    // Object-to-world transform, updated by applyTransform and sent to the vertex shader.
    glm::mat4 model{1.0f};
};
