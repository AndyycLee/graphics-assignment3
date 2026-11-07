#include "mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<Vertex> makePositionVertices(const std::vector<glm::vec3>& positions)
{
    std::vector<Vertex> vertices;
    vertices.reserve(positions.size());
    for (const glm::vec3& position : positions) {
        Vertex vertex;
        vertex.position = position;
        vertex.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        vertices.push_back(vertex);
    }
    return vertices;
}

void flipImageVertically(unsigned char* pixels, int width, int height)
{
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 4;
    std::vector<unsigned char> row(rowBytes);
    for (int y = 0; y < height / 2; ++y) {
        unsigned char* top = pixels + static_cast<std::size_t>(y) * rowBytes;
        unsigned char* bottom = pixels + static_cast<std::size_t>(height - 1 - y) * rowBytes;
        std::copy_n(top, rowBytes, row.data());
        std::copy_n(bottom, rowBytes, top);
        std::copy_n(row.data(), rowBytes, bottom);
    }
}

void bindAttribute(GLuint program,
                   const char* name,
                   GLint componentCount,
                   GLsizei stride,
                   std::size_t offset)
{
    const GLint location = glGetAttribLocation(program, name);
    if (location < 0) {
        throw std::runtime_error(std::string("Mesh vertex shader has no '") + name +
                                 "' attribute");
    }
    const GLuint attribute = static_cast<GLuint>(location);
    glEnableVertexAttribArray(attribute);
    glVertexAttribPointer(attribute,
                          componentCount,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          reinterpret_cast<const void*>(offset));
}
}

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices,
           const std::string& vertexShaderPath,
           const std::string& fragmentShaderPath,
           const std::string& texturePath,
           const Material& material)
    : shader(vertexShaderPath, fragmentShaderPath),
      material(material)
{
    if (vertices.empty() || indices.empty() ||
        indices.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())) {
        throw std::invalid_argument("A triangle mesh requires vertices and a valid index count");
    }
    if (material.shininess <= 0.0f) {
        throw std::invalid_argument("Material shininess must be greater than zero");
    }
    indexCount = static_cast<GLsizei>(indices.size());

    for (const unsigned int index : indices) {
        if (index >= vertices.size()) {
            throw std::invalid_argument("Mesh index is outside the vertex list");
        }
    }

    std::unique_ptr<unsigned char, decltype(&stbi_image_free)> image(nullptr, stbi_image_free);
    int imageWidth = 0;
    int imageHeight = 0;
    if (!texturePath.empty()) {
        image.reset(stbi_load(texturePath.c_str(), &imageWidth, &imageHeight, nullptr, STBI_rgb_alpha));
        if (!image) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Unable to load texture '" + texturePath + "': " +
                                     (reason ? reason : "unknown image error"));
        }
        flipImageVertically(image.get(), imageWidth, imageHeight);
    }

    const GLuint program = shader.id();
    // Resolve all required attributes before allocating any OpenGL resources.
    const GLint position = glGetAttribLocation(program, "position");
    if (position < 0) {
        throw std::runtime_error("Mesh vertex shader has no 'position' attribute");
    }
    for (const char* attribute : {"normal", "texCoord", "color"}) {
        if (glGetAttribLocation(program, attribute) < 0) {
            throw std::runtime_error(std::string("Mesh vertex shader has no '") + attribute +
                                     "' attribute");
        }
    }

    glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);

    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    const GLsizei vertexStride = static_cast<GLsizei>(sizeof(Vertex));
    bindAttribute(program, "position", 3, vertexStride, offsetof(Vertex, position));
    bindAttribute(program, "normal", 3, vertexStride, offsetof(Vertex, normal));
    bindAttribute(program, "texCoord", 2, vertexStride, offsetof(Vertex, texCoord));
    bindAttribute(program, "color", 4, vertexStride, offsetof(Vertex, color));

    // The element buffer stores the vertex-list indices in triangle order.
    glGenBuffers(1, &indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    if (image) {
        glGenTextures(1, &texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA8,
                     imageWidth,
                     imageHeight,
                     0,
                     GL_RGBA,
                     GL_UNSIGNED_BYTE,
                     image.get());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Mesh::Mesh(const std::vector<glm::vec3>& positions,
           const std::vector<unsigned int>& indices,
           const std::string& vertexShaderPath,
           const std::string& fragmentShaderPath,
           const std::string& texturePath,
           const Material& material)
    : Mesh(makePositionVertices(positions),
           indices,
           vertexShaderPath,
           fragmentShaderPath,
           texturePath,
           material)
{
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    if (texture != 0) {
        glDeleteTextures(1, &texture);
    }
}

void Mesh::applyTransform(const glm::mat4& transform)
{
    model *= transform;
}

void Mesh::draw(const glm::mat4& view, const glm::mat4& projection) const
{
    shader.use();
    shader.setMatrix4("model", model);
    shader.setMatrix4("view", view);
    shader.setMatrix4("projection", projection);
    shader.setVector3("materialAmbient", material.ambient);
    shader.setVector3("materialDiffuse", material.diffuse);
    shader.setVector3("materialSpecular", material.specular);
    shader.setFloat("materialShininess", material.shininess);
    shader.setInt("useLighting", material.lightingEnabled ? 1 : 0);
    shader.setInt("useTexture", texture == 0 ? 0 : 1);
    shader.setInt("diffuseTexture", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vertexArray);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
