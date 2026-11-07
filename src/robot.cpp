#include <glad/gl.h> // must be included before the window headers
#include <SFML/Window.hpp>
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
constexpr float pi = 3.1415926535897932f;
// each group of three indices selects one triangle from the vertex list
struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

Vertex makeVertex(const glm::vec3& position, const glm::vec3& normal,
                  const glm::vec2& uv = glm::vec2(0.0f))
{
    Vertex result;
    result.position = position;
    result.normal = normal;
    result.texCoord = uv;
    return result;
}

Geometry makeCuboid(float width, float height, float depth)
{
    const float x = width / 2.0f, y = height / 2.0f, z = depth / 2.0f;
    const glm::vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const glm::vec3 positions[6][4] = {
        {{-x,-y,z},{x,-y,z},{x,y,z},{-x,y,z}},
        {{x,-y,-z},{-x,-y,-z},{-x,y,-z},{x,y,-z}},
        {{x,-y,z},{x,-y,-z},{x,y,-z},{x,y,z}},
        {{-x,-y,-z},{-x,-y,z},{-x,y,z},{-x,y,-z}},
        {{-x,y,z},{x,y,z},{x,y,-z},{-x,y,-z}},
        {{-x,-y,-z},{x,-y,-z},{x,-y,z},{-x,-y,z}}
    };
    const glm::vec3 normals[6] = {{0,0,1},{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0}};
    Geometry result;
    for (unsigned int face = 0; face < 6; ++face) {
        const unsigned int first = static_cast<unsigned int>(result.vertices.size());
        // separate vertices let each face keep its own normal at sharp edges
        for (unsigned int corner = 0; corner < 4; ++corner)
            result.vertices.push_back(makeVertex(positions[face][corner], normals[face], uv[corner]));
        result.indices.insert(result.indices.end(), {first, first + 1, first + 2,
                                                   first, first + 2, first + 3});
    }
    return result;
}

Geometry makeCylinder(float radius, float height, unsigned int segments = 48,
                      float sideURepeat = 1.0f, bool includeCaps = true,
                      float sideVRepeat = 1.0f)
{
    Geometry result;
    // duplicate the seam position so its two sides can use different u values
    for (unsigned int i = 0; i <= segments; ++i) {
        const float angleU = static_cast<float>(i) / segments;
        const float u = sideURepeat * angleU;
        const float angle = 2.0f * pi * angleU;
        // the cylinder extends along y, so side normals point outward in the xz plane
        const glm::vec3 normal(std::cos(angle), 0.0f, std::sin(angle));
        const unsigned int first = static_cast<unsigned int>(result.vertices.size());
        result.vertices.push_back(makeVertex({radius * normal.x, -height / 2, radius * normal.z},
                                          normal, {u, 0}));
        result.vertices.push_back(makeVertex({radius * normal.x, height / 2, radius * normal.z},
                                          normal, {u, sideVRepeat}));
        if (i == segments)
            break;
        const unsigned int next = first + 2;
        result.indices.insert(result.indices.end(), {first, first + 1, next,
                                                   first + 1, next + 1, next});
    }
    if (!includeCaps)
        return result;

    // caps need their own vertices because their normals point up or down
    const unsigned int topCenter = static_cast<unsigned int>(result.vertices.size());
    const unsigned int bottomCenter = topCenter + 1;
    result.vertices.push_back(makeVertex({0, height / 2, 0}, {0, 1, 0}, glm::vec2(0.5f)));
    result.vertices.push_back(makeVertex({0, -height / 2, 0}, {0, -1, 0}, glm::vec2(0.5f)));
    const unsigned int topRing = static_cast<unsigned int>(result.vertices.size());
    for (unsigned int i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / segments;
        const float angle = 2.0f * pi * u;
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec2 capUv(0.5f + radial.x * 0.5f, 0.5f + radial.z * 0.5f);
        result.vertices.push_back(makeVertex({radius * radial.x, height / 2, radius * radial.z},
                                          {0, 1, 0}, capUv));
    }
    const unsigned int bottomRing = static_cast<unsigned int>(result.vertices.size());
    for (unsigned int i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / segments;
        const float angle = 2.0f * pi * u;
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec2 capUv(0.5f + radial.x * 0.5f, 0.5f + radial.z * 0.5f);
        result.vertices.push_back(makeVertex({radius * radial.x, -height / 2, radius * radial.z},
                                          {0, -1, 0}, capUv));
    }
    for (unsigned int i = 0; i < segments; ++i) {
        result.indices.insert(result.indices.end(),
                             {topCenter, topRing + i + 1, topRing + i,
                              bottomCenter, bottomRing + i, bottomRing + i + 1});
    }
    return result;
}

Geometry makeTopDisk(float radius, float height, unsigned int segments = 48,
                     bool top = true)
{
    Geometry result;
    const float sign = top ? 1.0f : -1.0f;
    result.vertices.push_back(makeVertex({0, sign * height / 2, 0}, {0, sign, 0}, glm::vec2(0.5f)));
    for (unsigned int i = 0; i <= segments; ++i) {
        const float u = static_cast<float>(i) / segments;
        const float angle = 2.0f * pi * u;
        const glm::vec3 radial(std::cos(angle), 0.0f, std::sin(angle));
        result.vertices.push_back(makeVertex({radius * radial.x, sign * height / 2, radius * radial.z},
                                          {0, sign, 0},
                                          glm::vec2(0.5f + radial.x * 0.5f,
                                                    0.5f + radial.z * 0.5f)));
    }
    for (unsigned int i = 0; i < segments; ++i) {
        if (top)
            result.indices.insert(result.indices.end(), {0, i + 2, i + 1});
        else
            result.indices.insert(result.indices.end(), {0, i + 1, i + 2});
    }
    return result;
}

Geometry makeSphere(unsigned int slices = 32, unsigned int stacks = 16)
{
    Geometry result;
    // sample latitude and longitude on a unit sphere centered at the origin
    for (unsigned int stack = 0; stack <= stacks; ++stack) {
        const float v = static_cast<float>(stack) / stacks;
        const float theta = pi * v;
        for (unsigned int slice = 0; slice <= slices; ++slice) {
            const float u = static_cast<float>(slice) / slices;
            const float phi = 2.0f * pi * u;
            const glm::vec3 normal(std::sin(theta) * std::cos(phi), std::cos(theta),
                                   std::sin(theta) * std::sin(phi));
            // on a unit sphere, the position is also the outward unit normal
            result.vertices.push_back(makeVertex(normal, normal, {u, 1.0f - v}));
        }
    }
    const unsigned int rowLength = slices + 1;
    for (unsigned int stack = 0; stack < stacks; ++stack) {
        for (unsigned int slice = 0; slice < slices; ++slice) {
            const unsigned int topLeft = stack * rowLength + slice;
            const unsigned int bottomLeft = topLeft + rowLength;
            result.indices.insert(result.indices.end(),
                                 {topLeft, topLeft + 1, bottomLeft,
                                  topLeft + 1, bottomLeft + 1, bottomLeft});
        }
    }
    return result;
}

struct JointAngles {
    float baseRoll = 0.0f;
    float shoulder = 0.0f;
    float elbow = 0.0f;
    float wrist = 0.0f;
    float claw = 0.0f;
    void reset() { *this = {}; }
};

struct PlacedPart {
    Mesh* mesh;
    glm::mat4 transform;
};

void place(PlacedPart part)
{
    // replace the previous pose instead of accumulating transforms across frames
    part.mesh->setTransform(part.transform);
}

glm::mat4 translation(float x, float y, float z)
{
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}

glm::mat4 zRotation(float radians)
{
    return glm::rotate(glm::mat4(1.0f), radians, glm::vec3(0, 0, 1));
}
}

int main(int argc, char* argv[])
{
    try {
        sf::ContextSettings settings;
        settings.majorVersion = 4;
        settings.minorVersion = 0;
        settings.attributeFlags = sf::ContextSettings::Core;
        settings.depthBits = 24;
        sf::Window window(sf::VideoMode({1000, 800}), "Procedural Robotic Arm",
                          sf::Style::Default, sf::State::Windowed, settings);
        if (!gladLoadGL(sf::Context::getFunction))
            throw std::runtime_error("Failed to initialize GLAD");
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glClearColor(0.055f, 0.07f, 0.10f, 1.0f);

        if (argc == 0)
            throw std::runtime_error("Unable to determine executable path");
        const std::filesystem::path executableDirectory =
            std::filesystem::absolute(argv[0]).parent_path();
        const std::string vertexShader = (executableDirectory / "shaders" / "mesh.vert").string();
        const std::string fragmentShader = (executableDirectory / "shaders" / "mesh.frag").string();
        const std::filesystem::path assets = executableDirectory / "assets";
        const std::string armTexture = (assets / "texture_arm_linkage.png").string();
        const std::string baseTexture = (assets / "texture_base_top.png").string();

        Material metal;
        metal.ambient = {0.18f, 0.20f, 0.24f};
        metal.diffuse = {0.42f, 0.48f, 0.48f};
        metal.specular = {0.75f, 0.75f, 0.75f};
        metal.shininess = 78.0f;
        Material joint;
        joint.ambient = {0.22f, 0.14f, 0.05f};
        joint.diffuse = {0.65f, 0.48f, 0.20f};
        joint.specular = {0.4f, 0.4f, 0.4f};
        joint.shininess = 32.0f;
        Material dark;
        dark.ambient = {0.04f, 0.04f, 0.04f};
        dark.diffuse = {0.11f, 0.11f, 0.11f};
        dark.specular = {0.45f, 0.45f, 0.45f};

        auto create = [&](Geometry geometry, const std::string& texture, const Material& material) {
            return Mesh(geometry.vertices, geometry.indices, vertexShader, fragmentShader,
                        texture, material);
        };
        Mesh baseSide = create(makeCylinder(1.1f, 0.7f, 48, 1.0f, false), {}, metal);
        Mesh baseTop = create(makeTopDisk(1.1f, 0.7f), baseTexture, metal);
        Mesh baseBottom = create(makeTopDisk(1.1f, 0.7f, 48, false), {}, metal);
        // equal texture density around and along the cylinder keeps dots round
        Mesh upperArm = create(makeCylinder(0.22f, 2.8f, 48, 1.0f, true,
                                            2.8f / (2.0f * pi * 0.22f)), armTexture, metal);
        Mesh forearm = create(makeCylinder(0.17f, 2.25f, 48, 1.0f, true,
                                           2.25f / (2.0f * pi * 0.17f)), armTexture, metal);
        Mesh palm = create(makeCuboid(0.82f, 0.68f, 0.36f), {}, metal);
        Mesh shoulder = create(makeSphere(), {}, joint);
        Mesh elbow = create(makeSphere(), {}, joint);
        Mesh wrist = create(makeSphere(), {}, joint);
        Mesh stemLeft = create(makeCylinder(0.045f, 0.58f), {}, dark);
        Mesh stemRight = create(makeCylinder(0.045f, 0.58f), {}, dark);
        Mesh fingerJointLeft = create(makeSphere(20, 10), {}, joint);
        Mesh fingerJointRight = create(makeSphere(20, 10), {}, joint);
        Mesh fingerLeft = create(makeCuboid(0.085f, 0.8f, 0.07f), {}, dark);
        Mesh fingerRight = create(makeCuboid(0.085f, 0.8f, 0.07f), {}, dark);

        JointAngles angles;
        constexpr float baseHeight = 0.7f;
        constexpr float upperArmLength = 2.8f;
        constexpr float forearmLength = 2.25f;
        constexpr float palmLength = 0.68f;
        constexpr float stemLength = 0.58f;

        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>())
                    window.close();
                if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                    // store angles in radians and clamp each joint to its allowed range
                    const float step = glm::radians(5.0f);
                    switch (key->code) {
                    case sf::Keyboard::Key::Left:
                    case sf::Keyboard::Key::A: angles.baseRoll = std::remainder(angles.baseRoll + step, 2.0f * pi); break;
                    case sf::Keyboard::Key::Right:
                    case sf::Keyboard::Key::D: angles.baseRoll = std::remainder(angles.baseRoll - step, 2.0f * pi); break;
                    case sf::Keyboard::Key::W: angles.shoulder = std::min(angles.shoulder + step, glm::radians(85.0f)); break;
                    case sf::Keyboard::Key::S: angles.shoulder = std::max(angles.shoulder - step, glm::radians(-25.0f)); break;
                    case sf::Keyboard::Key::R: angles.elbow = std::min(angles.elbow + step, glm::radians(125.0f)); break;
                    case sf::Keyboard::Key::F: angles.elbow = std::max(angles.elbow - step, glm::radians(-10.0f)); break;
                    case sf::Keyboard::Key::T: angles.wrist = std::min(angles.wrist + step, glm::radians(75.0f)); break;
                    case sf::Keyboard::Key::G: angles.wrist = std::max(angles.wrist - step, glm::radians(-75.0f)); break;
                    case sf::Keyboard::Key::O: angles.claw = std::min(angles.claw + step, glm::radians(28.0f)); break;
                    case sf::Keyboard::Key::C: angles.claw = std::max(angles.claw - step, 0.0f); break;
                    case sf::Keyboard::Key::Space: angles.reset(); break;
                    case sf::Keyboard::Key::Escape: window.close(); break;
                    default: break;
                    }
                }
            }
            if (!window.isOpen())
                break;

            // each child starts from its parent's frame to inherit its movement
            const glm::mat4 root = glm::rotate(glm::mat4(1.0f), angles.baseRoll,
                                               glm::vec3(0, 1, 0));
            const glm::mat4 shoulderFrame = root * translation(0, baseHeight, 0);
            const glm::mat4 upperFrame = shoulderFrame * zRotation(angles.shoulder);
            // move along the rotated upper arm to reach the elbow pivot
            const glm::mat4 elbowFrame = upperFrame * translation(0, upperArmLength, 0);
            const glm::mat4 forearmFrame = elbowFrame * zRotation(angles.elbow);
            const glm::mat4 wristFrame = forearmFrame * translation(0, forearmLength, 0);
            const glm::mat4 handFrame = wristFrame * zRotation(angles.wrist);
            const glm::mat4 palmFrame = handFrame * translation(0, palmLength / 2, 0);
            const glm::mat4 stemFrame = handFrame * translation(0, palmLength, 0);

            const glm::mat4 baseTransform = root * translation(0, baseHeight / 2, 0);
            place({&baseSide, baseTransform});
            place({&baseTop, baseTransform});
            place({&baseBottom, baseTransform});
            place({&shoulder, shoulderFrame * glm::scale(glm::mat4(1), glm::vec3(0.24f))});
            // cylinders are centered at the origin, so offset their centers by half their length
            place({&upperArm, upperFrame * translation(0, upperArmLength / 2, 0)});
            place({&elbow, elbowFrame * glm::scale(glm::mat4(1), glm::vec3(0.20f))});
            place({&forearm, forearmFrame * translation(0, forearmLength / 2, 0)});
            place({&wrist, wristFrame * glm::scale(glm::mat4(1), glm::vec3(0.17f))});
            place({&stemLeft, stemFrame * translation(-0.22f, stemLength / 2, 0)});
            place({&stemRight, stemFrame * translation(0.22f, stemLength / 2, 0)});
            place({&palm, palmFrame});

            const float fingerOffset = 0.22f;
            const glm::mat4 leftJointFrame = stemFrame * translation(-fingerOffset, stemLength, 0);
            const glm::mat4 rightJointFrame = stemFrame * translation(fingerOffset, stemLength, 0);
            // the center offset is applied first, then rotated around the joint
            // opposite signs move the two fingertips away from each other when opening
            const glm::mat4 leftFrame = leftJointFrame * zRotation(angles.claw);
            const glm::mat4 rightFrame = rightJointFrame * zRotation(-angles.claw);
            place({&fingerJointLeft, leftJointFrame *
                   glm::scale(glm::mat4(1), glm::vec3(0.06f))});
            place({&fingerJointRight, rightJointFrame *
                   glm::scale(glm::mat4(1), glm::vec3(0.06f))});
            place({&fingerLeft, leftFrame * translation(0, 0.4f, 0)});
            place({&fingerRight, rightFrame * translation(0, 0.4f, 0)});

            const sf::Vector2u size = window.getSize();
            glViewport(0, 0, static_cast<GLsizei>(size.x), static_cast<GLsizei>(size.y));
            if (size.x == 0 || size.y == 0)
                continue;
            const float aspect = static_cast<float>(size.x) / size.y;

            // fixed camera aimed at the shoulder, with eight units covering the arm's full reach
            const glm::mat4 view = glm::lookAt(glm::vec3(10, 8, 14),
                                               glm::vec3(0, baseHeight, 0), glm::vec3(0, 1, 0));
            // expand the longer window dimension so resizing never crops the arm
            const float halfWidth = 8.0f * std::max(1.0f, aspect);
            const float halfHeight = 8.0f * std::max(1.0f, 1.0f / aspect);
            const glm::mat4 projection = glm::ortho(
                -halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 50.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (Mesh* mesh : {&baseSide, &baseTop, &baseBottom, &shoulder, &upperArm, &elbow, &forearm, &wrist,
                               &stemLeft, &stemRight, &palm, &fingerJointLeft, &fingerJointRight,
                               &fingerLeft, &fingerRight})
                mesh->draw(view, projection);
            window.display();
        }
    } catch (const std::exception& error) {
        std::cerr << "Application failed: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
