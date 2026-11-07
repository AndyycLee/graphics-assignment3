#include <glad/gl.h> // Must be first!
#include <SFML/Window.hpp>
#include <SFML/System/Clock.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "mesh.h"

namespace {
struct CubeGeometry {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

Vertex makeVertex(const glm::vec3& position, const glm::vec2& texCoord)
{
    Vertex vertex;
    vertex.position = position;
    vertex.texCoord = texCoord;
    return vertex;
}

CubeGeometry makeCube()
{
    constexpr glm::vec2 bottomLeft(0.0f, 0.0f);
    constexpr glm::vec2 bottomRight(1.0f, 0.0f);
    constexpr glm::vec2 topRight(1.0f, 1.0f);
    constexpr glm::vec2 topLeft(0.0f, 1.0f);

    CubeGeometry cube;
    cube.vertices = {
        // Front
        makeVertex({-1.0f, -1.0f,  1.0f}, bottomLeft),
        makeVertex({ 1.0f, -1.0f,  1.0f}, bottomRight),
        makeVertex({ 1.0f,  1.0f,  1.0f}, topRight),
        makeVertex({-1.0f,  1.0f,  1.0f}, topLeft),
        // Back
        makeVertex({ 1.0f, -1.0f, -1.0f}, bottomLeft),
        makeVertex({-1.0f, -1.0f, -1.0f}, bottomRight),
        makeVertex({-1.0f,  1.0f, -1.0f}, topRight),
        makeVertex({ 1.0f,  1.0f, -1.0f}, topLeft),
        // Right
        makeVertex({ 1.0f, -1.0f,  1.0f}, bottomLeft),
        makeVertex({ 1.0f, -1.0f, -1.0f}, bottomRight),
        makeVertex({ 1.0f,  1.0f, -1.0f}, topRight),
        makeVertex({ 1.0f,  1.0f,  1.0f}, topLeft),
        // Left
        makeVertex({-1.0f, -1.0f, -1.0f}, bottomLeft),
        makeVertex({-1.0f, -1.0f,  1.0f}, bottomRight),
        makeVertex({-1.0f,  1.0f,  1.0f}, topRight),
        makeVertex({-1.0f,  1.0f, -1.0f}, topLeft),
        // Top
        makeVertex({-1.0f,  1.0f,  1.0f}, bottomLeft),
        makeVertex({ 1.0f,  1.0f,  1.0f}, bottomRight),
        makeVertex({ 1.0f,  1.0f, -1.0f}, topRight),
        makeVertex({-1.0f,  1.0f, -1.0f}, topLeft),
        // Bottom
        makeVertex({-1.0f, -1.0f, -1.0f}, bottomLeft),
        makeVertex({ 1.0f, -1.0f, -1.0f}, bottomRight),
        makeVertex({ 1.0f, -1.0f,  1.0f}, topRight),
        makeVertex({-1.0f, -1.0f,  1.0f}, topLeft)
    };

    cube.indices.reserve(36);
    for (unsigned int face = 0; face < 6; ++face) {
        const unsigned int first = face * 4;
        cube.indices.insert(cube.indices.end(),
                            {first, first + 1, first + 2,
                             first, first + 2, first + 3});
    }

    return cube;
}

void rotate(Mesh& mesh, float radians)
{
    mesh.applyTransform(glm::rotate(glm::mat4(1.0f),
                                    radians,
                                    glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f))));
}
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: cube <texture-image>\n";
        return 2;
    }

    try {
        sf::ContextSettings settings;
        settings.majorVersion = 4;
        settings.minorVersion = 0;
        settings.attributeFlags = sf::ContextSettings::Core;
        settings.depthBits = 24;

        sf::Window window(sf::VideoMode({800, 800}),
                          "Textured cube",
                          sf::Style::Default,
                          sf::State::Windowed,
                          settings);

        const sf::ContextSettings actualSettings = window.getSettings();
        if (actualSettings.majorVersion < 4) {
            throw std::runtime_error("OpenGL 4.0 Core is required, but the created context is " +
                                     std::to_string(actualSettings.majorVersion) + "." +
                                     std::to_string(actualSettings.minorVersion));
        }

        if (!gladLoadGL(sf::Context::getFunction)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            return 1;
        }

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        const CubeGeometry cube = makeCube();
        const std::filesystem::path shaderDirectory =
            std::filesystem::absolute(argv[0]).parent_path() / "shaders";
        Material material;
        material.lightingEnabled = false;
        Mesh mesh(cube.vertices,
                  cube.indices,
                  (shaderDirectory / "mesh.vert").string(),
                  (shaderDirectory / "mesh.frag").string(),
                  argv[1],
                  material);

        constexpr float fieldOfView = glm::radians(45.0f);
        constexpr float cubeRadius = 1.73205080757f;
        const glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        sf::Clock frameClock;
        const float rotationSpeed = glm::radians(30.0f);

        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    glViewport(0,
                               0,
                               static_cast<GLsizei>(resized->size.x),
                               static_cast<GLsizei>(resized->size.y));
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::Left) {
                        rotate(mesh, glm::radians(5.0f));
                    } else if (keyPressed->code == sf::Keyboard::Key::Right) {
                        rotate(mesh, glm::radians(-5.0f));
                    } else if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
            }

            if (!window.isOpen()) {
                break;
            }

            const float dt = frameClock.restart().asSeconds();
            rotate(mesh, rotationSpeed * dt);

            const sf::Vector2u size = window.getSize();
            glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
            const float aspectRatio = size.x == 0 || size.y == 0
                ? 1.0f
                : static_cast<float>(size.x) / static_cast<float>(size.y);
            const float limitingAspect = std::min(aspectRatio, 1.0f);
            const float limitingHalfFov = std::atan(
                std::tan(fieldOfView * 0.5f) * limitingAspect);
            const float cameraDistance = cubeRadius / std::sin(limitingHalfFov) * 1.1f;
            const glm::mat4 view = glm::lookAt(cameraTarget + glm::vec3(0.0f,
                                                                        0.0f,
                                                                        cameraDistance),
                                               cameraTarget,
                                               worldUp);
            const glm::mat4 projection = glm::perspective(fieldOfView,
                                                           aspectRatio,
                                                           0.1f,
                                                           cameraDistance + cubeRadius * 2.0f);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            mesh.draw(view, projection);
            window.display();
        }
    } catch (const std::exception& error) {
        std::cerr << "Application failed: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}
