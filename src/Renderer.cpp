#include "Renderer.h"

#include "Animation.h"
#include "Camera.h"
#include "CashInteraction.h"
#include "Payment.h"
#include <glm/gtc/quaternion.hpp>

#include "VendingMachine.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Face {
    std::array<int, 4> corners;
    glm::vec3 normal;
};

constexpr std::array<glm::vec3, 8> cubeCorners{{
    {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
    {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
    {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
}};

constexpr std::array<Face, 6> cubeFaces{{
    {{{4, 5, 6, 7}}, {0.0f, 0.0f, 1.0f}},
    {{{1, 0, 3, 2}}, {0.0f, 0.0f, -1.0f}},
    {{{0, 4, 7, 3}}, {-1.0f, 0.0f, 0.0f}},
    {{{5, 1, 2, 6}}, {1.0f, 0.0f, 0.0f}},
    {{{3, 7, 6, 2}}, {0.0f, 1.0f, 0.0f}},
    {{{0, 1, 5, 4}}, {0.0f, -1.0f, 0.0f}},
}};

void appendVertex(std::vector<float>& data, const glm::vec3& position,
                  const glm::vec3& normal) {
    data.insert(data.end(), {position.x, position.y, position.z,
                             normal.x, normal.y, normal.z});
}

std::pair<GLint, GLint> appendCylinder(std::vector<float>& data, float bottomRadius,
                                       float topRadius, int segments) {
    const GLint first = static_cast<GLint>(data.size() / 6);
    for (int i = 0; i < segments; ++i) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(i) /
                         static_cast<float>(segments);
        const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) /
                         static_cast<float>(segments);
        const glm::vec3 bottom0{bottomRadius * std::cos(a0), -0.5f,
                                bottomRadius * std::sin(a0)};
        const glm::vec3 bottom1{bottomRadius * std::cos(a1), -0.5f,
                                bottomRadius * std::sin(a1)};
        const glm::vec3 top0{topRadius * std::cos(a0), 0.5f,
                             topRadius * std::sin(a0)};
        const glm::vec3 top1{topRadius * std::cos(a1), 0.5f,
                             topRadius * std::sin(a1)};
        const glm::vec3 normal0 =
            glm::normalize(glm::vec3(std::cos(a0), bottomRadius - topRadius,
                                     std::sin(a0)));
        const glm::vec3 normal1 =
            glm::normalize(glm::vec3(std::cos(a1), bottomRadius - topRadius,
                                     std::sin(a1)));
        appendVertex(data, bottom0, normal0);
        appendVertex(data, top0, normal0);
        appendVertex(data, top1, normal1);
        appendVertex(data, bottom0, normal0);
        appendVertex(data, top1, normal1);
        appendVertex(data, bottom1, normal1);

        appendVertex(data, {0.0f, -0.5f, 0.0f}, {0.0f, -1.0f, 0.0f});
        appendVertex(data, bottom1, {0.0f, -1.0f, 0.0f});
        appendVertex(data, bottom0, {0.0f, -1.0f, 0.0f});
        appendVertex(data, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
        appendVertex(data, top0, {0.0f, 1.0f, 0.0f});
        appendVertex(data, top1, {0.0f, 1.0f, 0.0f});
    }
    return {first, static_cast<GLint>(data.size() / 6) - first};
}
}

Renderer::Renderer(const std::string& shaderDirectory) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, shaderDirectory + "/vertex.glsl");
    const GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, shaderDirectory + "/fragment.glsl");
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(program_, logLength, nullptr, log.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error("Shader program link failed: " + log);
    }

    mvpLocation_ = glGetUniformLocation(program_, "uMVP");
    modelLocation_ = glGetUniformLocation(program_, "uModel");
    colorLocation_ = glGetUniformLocation(program_, "uColor");
    lightLocation_ = glGetUniformLocation(program_, "uLightPosition");
    opacityLocation_ = glGetUniformLocation(program_, "uOpacity");
    emissiveLocation_ = glGetUniformLocation(program_, "uEmissive");

    std::vector<float> vertices;
    constexpr int triangleCorners[6] = {0, 1, 2, 0, 2, 3};
    vertices.reserve(36 * 6);
    for (const Face& face : cubeFaces) {
        for (const int corner : triangleCorners) {
            appendVertex(vertices, cubeCorners[static_cast<std::size_t>(
                                       face.corners[static_cast<std::size_t>(corner)])],
                         face.normal);
        }
    }
    // Tessellated rounded cube, reused for cabinet panels and controls.
    roundedFirst_ = static_cast<GLint>(vertices.size() / 6);
    for (const Face& face : cubeFaces) {
        const auto a = cubeCorners[face.corners[0]];
        const auto b = cubeCorners[face.corners[1]];
        const auto d = cubeCorners[face.corners[3]];
        constexpr int steps = 12;
        for (int y = 0; y < steps; ++y) {
            for (int x = 0; x < steps; ++x) {
                for (int corner : triangleCorners) {
                    const int dx = (corner == 1 || corner == 2) ? 1 : 0;
                    const int dy = (corner == 2 || corner == 3) ? 1 : 0;
                    const glm::vec3 point = a + (b - a) * (static_cast<float>(x + dx) / steps)
                                             + (d - a) * (static_cast<float>(y + dy) / steps);
                    const glm::vec3 inner = glm::clamp(point, glm::vec3(-0.43f), glm::vec3(0.43f));
                    const glm::vec3 normal = glm::normalize(point - inner);
                    appendVertex(vertices, inner + normal * 0.07f, normal);
                }
            }
        }
    }
    roundedCount_ = static_cast<GLint>(vertices.size() / 6) - roundedFirst_;
    const auto cylinderRange = appendCylinder(vertices, 0.5f, 0.5f, 32);
    cylinderFirst_ = cylinderRange.first;
    cylinderCount_ = cylinderRange.second;
    const auto frustumRange = appendCylinder(vertices, 0.5f, 0.24f, 32);
    frustumFirst_ = frustumRange.first;
    frustumCount_ = frustumRange.second;
    const auto coneRange = appendCylinder(vertices, 0.44f, 0.02f, 32);
    coneFirst_ = coneRange.first;
    coneCount_ = coneRange.second;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

Renderer::~Renderer() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

std::string Renderer::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open shader file: " + path);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

GLuint Renderer::compileShader(GLenum type, const std::string& path) {
    const std::string source = readFile(path);
    const char* sourcePointer = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile failed (" + path + "): " + log);
    }
    return shader;
}

void Renderer::drawMesh(const glm::mat4& viewProjection, const glm::vec3& position,
                        const glm::vec3& scale, const glm::vec3& color, GLint first,
                        GLint count, float opacity, float emissive) {
    const glm::mat4 model = parentTransform_ * glm::translate(glm::mat4(1.0f), position) *
                            glm::scale(glm::mat4(1.0f), scale);
    const glm::mat4 mvp = viewProjection * model;
    glUniformMatrix4fv(modelLocation_, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(colorLocation_, 1, &color[0]);
    glUniform1f(opacityLocation_, opacity);
    glUniform1f(emissiveLocation_, emissive);
    glDrawArrays(GL_TRIANGLES, first, count);
}

void Renderer::drawBox(const glm::mat4& viewProjection, const glm::vec3& position,
                       const glm::vec3& scale, const glm::vec3& color, float emissive) {
    const bool smallDetail = scale.x < 0.035f || scale.y < 0.035f;
    drawMesh(viewProjection, position, scale, color,
             smallDetail ? 0 : roundedFirst_, smallDetail ? 36 : roundedCount_, 1.0f, emissive);
}

void Renderer::drawCylinder(const glm::mat4& viewProjection, const glm::vec3& position,
                            const glm::vec3& scale, const glm::vec3& color,
                            bool tapered) {
    drawMesh(viewProjection, position, scale, color,
             tapered ? frustumFirst_ : cylinderFirst_,
             tapered ? frustumCount_ : cylinderCount_);
}

// Small geometry font: labels stay attached to the cabinet when orbiting.
void Renderer::drawText(const glm::mat4& vp, const std::string& text,
                        glm::vec3 center, float height, glm::vec3 color) {
    static const std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.$-";
    static const std::array<std::array<int, 7>, 39> glyphs{{
        {{14,17,17,31,17,17,17}},{{30,17,17,30,17,17,30}},
        {{14,17,16,16,16,17,14}},{{30,17,17,17,17,17,30}},
        {{31,16,16,30,16,16,31}},{{31,16,16,30,16,16,16}},
        {{14,17,16,23,17,17,14}},{{17,17,17,31,17,17,17}},
        {{31,4,4,4,4,4,31}},{{7,2,2,2,18,18,12}},
        {{17,18,20,24,20,18,17}},{{16,16,16,16,16,16,31}},
        {{17,27,21,21,17,17,17}},{{17,25,21,19,17,17,17}},
        {{14,17,17,17,17,17,14}},{{30,17,17,30,16,16,16}},
        {{14,17,17,17,21,18,13}},{{30,17,17,30,20,18,17}},
        {{15,16,16,14,1,1,30}},{{31,4,4,4,4,4,4}},
        {{17,17,17,17,17,17,14}},{{17,17,17,17,17,10,4}},
        {{17,17,17,21,21,21,10}},{{17,17,10,4,10,17,17}},
        {{17,17,10,4,4,4,4}},{{31,1,2,4,8,16,31}},
        {{14,17,19,21,25,17,14}},{{4,12,4,4,4,4,14}},
        {{14,17,1,2,4,8,31}},{{30,1,1,14,1,1,30}},
        {{2,6,10,18,31,2,2}},{{31,16,16,30,1,1,30}},
        {{14,16,16,30,17,17,14}},{{31,1,2,4,8,8,8}},
        {{14,17,17,14,17,17,14}},{{14,17,17,15,1,1,14}},
        {{0,0,0,0,0,6,6}},{{4,15,20,14,5,30,4}},
        {{0,0,0,31,0,0,0}}
    }};
    const float pixel = height / 7.0f;
    const float left = center.x - (static_cast<float>(text.size()) * 6.0f - 1.0f) * pixel * 0.5f;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto index = alphabet.find(text[i]);
        if (index == std::string::npos) continue;
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (glyphs[index][row] & (1 << (4 - col)))
                    drawBox(vp, {left + (i * 6 + col) * pixel,
                                 center.y + (3 - row) * pixel, center.z},
                            {pixel * 0.94f, pixel * 0.94f, 0.006f}, color, 0.15f);
    }
}

void Renderer::drawProduct(const glm::mat4& viewProjection, const Product& product,
                           const glm::vec3& position, bool selected) {
    const float brightness = selected ? 1.16f : 1.0f;
    const glm::vec3 color = glm::min(product.color * brightness, glm::vec3(1.0f));
    switch (product.type) {
    case ProductType::Can: {
        drawCylinder(viewProjection, position, {0.34f, 0.58f, 0.34f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.294f, position.z},
                     {0.34f, 0.025f, 0.34f}, {0.66f, 0.70f, 0.70f});
        drawCylinder(viewProjection, {position.x, position.y - 0.294f, position.z},
                     {0.34f, 0.018f, 0.34f}, {0.56f, 0.59f, 0.59f});
        if (product.id == 1 || product.id == 3 || product.id == 11) {
            drawCylinder(viewProjection, {position.x, position.y + 0.02f,
                                           position.z},
                         {0.348f, 0.20f, 0.348f}, {0.93f, 0.91f, 0.84f});
            drawCylinder(viewProjection, {position.x, position.y - 0.105f,
                                           position.z},
                         {0.35f, 0.045f, 0.35f}, color);
        } else {
            drawCylinder(viewProjection, {position.x, position.y + 0.03f,
                                           position.z},
                         {0.348f, 0.16f, 0.348f}, {0.93f, 0.92f, 0.87f});
            drawCylinder(viewProjection, {position.x, position.y - 0.105f,
                                           position.z},
                         {0.35f, 0.055f, 0.35f}, {0.84f, 0.045f, 0.055f});
        }
        break;
    }
    case ProductType::Bottle: {
        drawCylinder(viewProjection, {position.x, position.y - 0.045f, position.z},
                     {0.30f, 0.43f, 0.30f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.245f, position.z},
                     {0.30f, 0.18f, 0.30f}, color, true);
        drawCylinder(viewProjection, {position.x, position.y + 0.36f, position.z},
                     {0.13f, 0.12f, 0.13f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.43f, position.z},
                     {0.15f, 0.055f, 0.15f},
                     product.id == 6 ? glm::vec3(0.90f, 0.25f, 0.05f)
                                     : glm::vec3(0.12f, 0.32f, 0.72f));
        drawCylinder(viewProjection, {position.x, position.y - 0.055f, position.z},
                     {0.307f, 0.15f, 0.307f}, {0.92f, 0.92f, 0.84f});
        break;
    }
    case ProductType::Cup:
        drawCylinder(viewProjection, {position.x, position.y - 0.025f, position.z},
                     {0.40f, 0.50f, 0.40f}, color, true);
        drawCylinder(viewProjection, {position.x, position.y + 0.235f, position.z},
                     {0.44f, 0.045f, 0.44f}, {0.96f, 0.94f, 0.86f});
        drawCylinder(viewProjection, {position.x, position.y + 0.27f, position.z},
                     {0.38f, 0.035f, 0.38f}, color);
        drawBox(viewProjection, {position.x, position.y + 0.13f,
                                 position.z + 0.205f},
                {0.23f, 0.12f, 0.015f}, {0.96f, 0.91f, 0.78f});
        break;
    case ProductType::Snack:
        drawBox(viewProjection, position, {0.40f, 0.53f, 0.28f}, color);
        drawBox(viewProjection, {position.x, position.y + 0.25f, position.z},
                {0.39f, 0.055f, 0.29f},
                glm::min(color * 0.74f, glm::vec3(1.0f)));
        drawBox(viewProjection, {position.x, position.y + 0.015f,
                                 position.z + 0.145f},
                {0.31f, 0.13f, 0.014f}, {0.96f, 0.88f, 0.67f});
        break;
    case ProductType::Bakery:
        drawBox(viewProjection, position, {0.43f, 0.48f, 0.30f}, color);
        drawBox(viewProjection, {position.x, position.y, position.z + 0.156f},
                {0.34f, 0.17f, 0.016f}, {0.94f, 0.87f, 0.70f});
        drawBox(viewProjection, {position.x, position.y + 0.225f, position.z},
                {0.44f, 0.035f, 0.31f},
                glm::min(color * 0.72f, glm::vec3(1.0f)));
        break;
    }
    if (selected) {
        drawBox(viewProjection, {position.x, position.y - 0.36f, position.z + 0.02f},
                {0.48f, 0.018f, 0.42f}, {0.20f, 0.96f, 0.86f}, 1.1f);
    }
}

void Renderer::render(const VendingMachine& machine, const Animation& animation,
                      const Camera& camera, const CashInteraction& cash, const Payment& payment, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.97f, 0.95f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 viewProjection =
        camera.projectionMatrix(static_cast<float>(width) / static_cast<float>(height)) *
        camera.viewMatrix();
    glUseProgram(program_);
    glUniform3f(lightLocation_, -3.5f, 8.0f, 5.0f);
    const auto eye = camera.position();
    glUniform3fv(glGetUniformLocation(program_, "uCameraPosition"), 1, &eye[0]);
    glBindVertexArray(vao_);

    const glm::vec3 mint{0.49f, 0.78f, 0.68f};
    const glm::vec3 mintLight{0.65f, 0.86f, 0.77f};
    const glm::vec3 cream{0.96f, 0.93f, 0.84f};
    const glm::vec3 deepMint{0.20f, 0.43f, 0.36f};

    // Warm studio floor and a soft, flattened contact shadow.
    drawBox(viewProjection, {0.0f, -0.16f, 0.0f}, {80.0f, 0.12f, 80.0f},
            {0.94f, 0.91f, 0.84f});
    drawCylinder(viewProjection, {0.08f, -0.085f, -0.06f}, {3.15f, 0.025f, 1.45f},
                 {0.72f, 0.69f, 0.62f});

    // Rounded-looking pastel shell, built from softly contrasting cabinet panels.
    drawBox(viewProjection, {0.0f, 2.72f, -0.72f}, {4.70f, 5.44f, 0.14f},
            mint);
    drawBox(viewProjection, {-2.27f, 2.72f, -0.02f}, {0.14f, 5.25f, 1.34f},
            mintLight);
    drawBox(viewProjection, {2.27f, 2.72f, -0.02f}, {0.12f, 5.25f, 1.32f},
            {0.40f, 0.70f, 0.60f});
    drawBox(viewProjection, {0.0f, 5.37f, -0.02f}, {4.60f, 0.14f, 1.35f},
            mint);
    drawBox(viewProjection, {0.0f, 0.07f, -0.02f}, {4.60f, 0.14f, 1.35f},
            mint);

    // Display chamber with a warm cream interior and a clean white surround.
    drawBox(viewProjection, {-0.43f, 2.78f, -0.70f}, {3.46f, 4.82f, 0.035f},
            {0.82f, 0.89f, 0.82f});
    drawBox(viewProjection, {-0.43f, 2.78f, -0.665f}, {3.30f, 4.66f, 0.035f},
            {0.89f, 0.91f, 0.84f});
    drawBox(viewProjection, {-2.05f, 2.76f, 0.015f}, {0.12f, 5.02f, 1.30f},
            mint);
    drawBox(viewProjection, {1.18f, 2.76f, 0.015f}, {0.12f, 5.02f, 1.30f},
            mint);

    // Three lit shelves; each shelf has four matching product positions.
    for (int shelf = 0; shelf < 3; ++shelf) {
        const float productY = 4.33f - static_cast<float>(shelf) * 1.32f;
        const float shelfY = productY - 0.40f;
        drawBox(viewProjection, {-0.43f, shelfY, 0.34f}, {3.18f, 0.075f, 0.88f},
                {0.83f, 0.86f, 0.78f});
        drawBox(viewProjection, {-0.43f, shelfY + 0.055f, 0.79f},
                {3.10f, 0.028f, 0.035f}, {0.94f, 0.98f, 0.91f}, 0.45f);
        drawBox(viewProjection, {-0.43f, shelfY - 0.09f, 0.80f},
                {3.08f, 0.13f, 0.035f}, {0.75f, 0.88f, 0.79f});
    }

    // Soft white interior LED strips.
    drawBox(viewProjection, {-1.97f, 2.78f, 0.57f}, {0.035f, 4.68f, 0.04f},
            {0.96f, 0.99f, 0.91f}, 0.85f);
    drawBox(viewProjection, {1.10f, 2.78f, 0.57f}, {0.035f, 4.68f, 0.04f},
            {0.96f, 0.99f, 0.91f}, 0.85f);

    for (int i = 0; i < static_cast<int>(machine.products().size()); ++i) {
        const Product& product = machine.products()[static_cast<std::size_t>(i)];
        glm::vec3 position = machine.productPosition(i);
        if (animation.active() && animation.productIndex() == i) {
            const float progress = animation.progress();
            position.x = glm::mix(position.x, -0.43f, progress);
            position.y = glm::mix(position.y, 0.50f, progress);
            position.z = 0.48f;
            drawProduct(viewProjection, product, position, false);
        } else if (product.stock > 0) {
            drawProduct(viewProjection, product, position, i == machine.selectedIndex());
        }
    }

    const std::array<const char*, 12> labels{{
        "COLA", "SODA", "ENERGY", "LEMON", "WATER", "ORANGE",
        "APPLE", "PEACH", "MILK TEA", "MILK", "COFFEE", "MATCHA"}};
    for (int i = 0; i < 12; ++i) {
        const auto position = machine.productPosition(i);
        const auto& product = machine.products()[static_cast<std::size_t>(i)];
        char tag[24];
        std::snprintf(tag, sizeof(tag), "%02d %.0fK", product.id, product.price / 1000.0f);
        drawBox(viewProjection, {position.x, position.y - 0.49f, 0.825f},
                {0.73f, 0.13f, 0.025f}, cream);
        drawText(viewProjection, tag, {position.x, position.y - 0.49f, 0.844f}, 0.075f, deepMint);
        if (product.stock > 0 && !(animation.active() && animation.productIndex() == i))
            drawText(viewProjection, labels[i], {position.x, position.y, 0.652f}, 0.065f, deepMint);
    }

    // Cream control column with a mint bezel around the screen and keypad.
    drawBox(viewProjection, {1.69f, 2.72f, 0.57f}, {0.93f, 4.84f, 0.24f},
            cream);
    drawBox(viewProjection, {1.69f, 4.60f, 0.72f}, {0.73f, 0.77f, 0.08f},
            deepMint);
    drawBox(viewProjection, {1.69f, 4.60f, 0.77f}, {0.65f, 0.68f, 0.035f},
            {0.77f, 0.91f, 0.80f});
    drawBox(viewProjection, {1.69f, 4.76f, 0.794f}, {0.48f, 0.035f, 0.012f},
            {0.23f, 0.54f, 0.43f}, 0.7f);
    drawBox(viewProjection, {1.69f, 4.60f, 0.794f}, {0.42f, 0.025f, 0.012f},
            {0.31f, 0.61f, 0.48f}, 0.55f);
    drawBox(viewProjection, {1.69f, 4.45f, 0.794f}, {0.48f, 0.025f, 0.012f},
            {0.31f, 0.61f, 0.48f}, 0.55f);

    drawText(viewProjection, "SELECT", {1.69f, 4.78f, 0.82f}, 0.085f, deepMint);
    drawText(viewProjection, "A DRINK", {1.69f, 4.61f, 0.82f}, 0.075f, deepMint);
    if (const Product* selected = machine.selectedProduct()) {
        char price[24];
        std::snprintf(price, sizeof(price), "%02d %.0fK", selected->id, selected->price / 1000.0f);
        drawText(viewProjection, price, {1.69f, 4.44f, 0.82f}, 0.065f, deepMint);
    }

    char credit[24];
    std::snprintf(credit, sizeof(credit), "%.0f VND", payment.balance());
    drawText(viewProjection, credit, {1.69f, 4.27f, 0.82f}, 0.060f, deepMint);

    for (int key = 0; key < 12; ++key) {
        const int row = key / 3;
        const int column = key % 3;
        const float x = 1.46f + static_cast<float>(column) * 0.22f;
        const float y = 3.70f - static_cast<float>(row) * 0.22f;
        drawBox(viewProjection, {x, y, 0.72f}, {0.15f, 0.15f, 0.08f},
                {0.77f, 0.87f, 0.79f});
        drawBox(viewProjection, {x, y, 0.765f}, {0.095f, 0.09f, 0.012f},
                {0.97f, 0.95f, 0.87f});
    }

    for (int i = 0; i < 12; ++i) {
        char number[4];
        std::snprintf(number, sizeof(number), "%02d", i + 1);
        drawText(viewProjection, number,
                 {1.46f + (i % 3) * 0.22f, 3.70f - (i / 3) * 0.22f, 0.78f},
                 0.09f, deepMint);
    }
    drawBox(viewProjection, {1.69f, 2.55f, 0.72f}, {0.51f, 0.06f, 0.09f},
            {0.23f, 0.27f, 0.24f});
    drawBox(viewProjection, {1.69f, 2.70f, 0.77f}, {0.51f, 0.035f, 0.02f},
            {0.72f, 0.77f, 0.72f});
    drawBox(viewProjection, {1.69f, 1.91f, 0.72f}, {0.53f, 0.13f, 0.10f},
            {0.79f, 0.88f, 0.80f});
    drawBox(viewProjection, {1.69f, 1.91f, 0.775f}, {0.34f, 0.026f, 0.018f},
            {0.35f, 0.44f, 0.37f});

    // Wide delivery bay at the base with a softly illuminated mint lip.
    drawBox(viewProjection, {-0.43f, 0.51f, 0.53f}, {1.62f, 0.55f, 0.72f},
            {0.44f, 0.61f, 0.52f});
    drawBox(viewProjection, {-0.43f, 0.52f, 0.68f}, {1.40f, 0.37f, 0.47f},
            {0.13f, 0.20f, 0.18f});
    drawBox(viewProjection, {-0.43f, 0.79f, 0.91f}, {1.38f, 0.045f, 0.055f},
            {0.87f, 0.98f, 0.86f}, 0.65f);

    // Hollow-looking delivery opening, framed by four separate cream rails.
    drawBox(viewProjection, {-0.43f, 0.51f, 0.94f}, {1.90f, 0.58f, 0.035f}, cream);
    drawBox(viewProjection, {-0.43f, 0.50f, 0.965f}, {1.66f, 0.39f, 0.025f}, {0.12f, 0.18f, 0.16f});
    drawBox(viewProjection, {-0.43f, 0.66f, 0.99f}, {1.62f, 0.12f, 0.04f}, cream);
    drawBox(viewProjection, {-0.43f, 0.70f, 1.02f}, {0.35f, 0.045f, 0.05f}, mintLight);
    for (float x : {-1.85f, 1.85f})
        for (float z : {-0.45f, 0.45f})
            drawCylinder(viewProjection, {x, -0.035f, z}, {0.22f, 0.13f, 0.22f}, deepMint);

    // A soft mint pane over the products suggests clear reflective glass.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawMesh(viewProjection, {-0.43f, 2.78f, 0.72f}, {3.10f, 4.58f, 0.018f},
             {0.79f, 0.95f, 0.86f}, 0, 36, 0.055f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Dedicated cream brand fascia above the glass.
    drawBox(viewProjection, {0.0f, 5.10f, 0.69f}, {4.35f, 0.48f, 0.12f}, cream);
    drawText(viewProjection, "MACHINE", {0.20f, 5.11f, 0.76f}, 0.26f, deepMint);
    drawCylinder(viewProjection, {-1.31f, 5.07f, 0.77f}, {0.19f, 0.17f, 0.07f}, mint);
    drawMesh(viewProjection, {-1.31f, 5.19f, 0.77f}, {0.19f, 0.20f, 0.07f},
             mint, coneFirst_, coneCount_);

    // Rounded corner bumpers and mint details complete the friendly silhouette.
    for (int side : {-1, 1}) {
        for (int end : {-1, 1}) {
            drawCylinder(viewProjection,
                         {static_cast<float>(side) * 2.20f,
                          end < 0 ? 0.20f : 5.23f, 0.64f},
                         {0.12f, 0.20f, 0.12f}, mint);
        }
    }

    drawCash(viewProjection, camera, cash);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Renderer::drawCash(const glm::mat4& vp, const Camera& camera, const CashInteraction& cash) {
    if (!cash.active()) return;
    const auto cameraFrame = glm::inverse(camera.viewMatrix());
    const auto heldFrame = cameraFrame * glm::translate(glm::mat4(1.0f), {0.35f, -0.34f, -0.95f});
    const glm::vec3 heldPosition(heldFrame[3]);
    const auto heldRotation = glm::quat_cast(glm::mat3(cameraFrame));
    const auto slotRotation = glm::angleAxis(glm::radians(86.0f), glm::vec3(1, 0, 0));
    const bool returning = cash.state() == CashInteraction::State::Returning;
    const float p = cash.progress();
    const auto ease = [](float t) { t = glm::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); };
    float reach = 0.0f;
    glm::vec3 slot{1.69f, returning ? 1.91f : 2.55f, 0.98f};
    if (cash.state() == CashInteraction::State::Inserting)
        reach = ease(p / 0.45f) * (1.0f - ease((p - 0.82f) / 0.18f));
    if (returning) reach = 1.0f - ease((p - 0.35f) / 0.65f);
    auto handPosition = glm::mix(heldPosition, slot, reach);
    const auto rotation = glm::slerp(heldRotation, slotRotation, reach);
    parentTransform_ = glm::translate(glm::mat4(1), handPosition) * glm::mat4_cast(rotation);
    const glm::vec3 skin{0.83f, 0.61f, 0.44f};
    // Hierarchical wrist, palm, four articulated fingers and thumb.
    drawBox(vp, {0, -0.30f, 0.07f}, {0.18f, 0.36f, 0.16f}, {0.27f, 0.49f, 0.42f});
    drawBox(vp, {0, -0.12f, 0.045f}, {0.22f, 0.22f, 0.10f}, skin);
    for (int finger = 0; finger < 4; ++finger) {
        const float x = -0.08f + finger * 0.052f;
        drawBox(vp, {x, 0.01f, 0.04f}, {0.043f, 0.13f, 0.05f}, skin);
        drawBox(vp, {x, 0.08f, 0.015f}, {0.044f, 0.055f, 0.075f}, skin);
    }
    drawBox(vp, {-0.13f, -0.015f, 0.09f}, {0.09f, 0.065f, 0.065f}, skin);
    // The bill moves independently after the hand reaches the slot.
    glm::vec3 billPosition = handPosition;
    if (cash.state() == CashInteraction::State::Inserting && p > 0.45f)
        billPosition = slot + glm::vec3(0, 0, -0.55f * ease((p - 0.45f) / 0.43f));
    if (returning && p < 0.35f)
        billPosition = slot + glm::vec3(0, 0, -0.40f * (1.0f - ease(p / 0.35f)));
    if (cash.state() != CashInteraction::State::Inserting || p < 0.90f) {
        const auto billRotation = cash.state() == CashInteraction::State::Inserting && p > 0.45f ? slotRotation : rotation;
        parentTransform_ = glm::translate(glm::mat4(1), billPosition) * glm::mat4_cast(billRotation);
        const auto noteFrame = parentTransform_;
        int remaining = cash.amount();
        const std::array<int, 6> denominations{{50000, 20000, 10000, 5000, 2000, 1000}};
        int noteCount = 0;
        do {
            int denomination = cash.amount();
            if (returning) {
                denomination = 1000;
                for (int value : denominations) if (remaining >= value) { denomination = value; break; }
            }
            parentTransform_ = noteFrame * glm::translate(glm::mat4(1),
                {noteCount * 0.015f, noteCount * 0.008f, noteCount * 0.012f});
            const glm::vec3 noteColor = denomination == 10000 ? glm::vec3(0.86f, 0.68f, 0.35f) :
                denomination == 20000 ? glm::vec3(0.45f, 0.65f, 0.88f) : glm::vec3(0.88f, 0.61f, 0.74f);
            drawBox(vp, {0, 0, 0.02f}, {0.56f, 0.25f, 0.008f}, noteColor);
            drawBox(vp, {0, 0, 0.026f}, {0.51f, 0.21f, 0.003f}, glm::mix(noteColor, glm::vec3(1), 0.24f));
            for (int i = 0; i < 5; ++i)
                drawBox(vp, {-0.23f + i * 0.022f, 0, 0.03f}, {0.008f, 0.18f, 0.002f}, noteColor * 0.8f);
            drawCylinder(vp, {0.17f, 0, 0.03f}, {0.09f, 0.11f, 0.01f}, noteColor);
            drawText(vp, std::to_string(denomination), {0.03f, 0.02f, 0.036f}, 0.052f, {0.20f, 0.29f, 0.24f});
            drawText(vp, "VND", {0.03f, -0.06f, 0.036f}, 0.035f, {0.20f, 0.29f, 0.24f});
            remaining -= denomination;
            ++noteCount;
        } while (returning && remaining > 0 && noteCount < 20);

    }
    parentTransform_ = glm::mat4(1.0f);
}
