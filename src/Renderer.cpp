#include "Renderer.h"

#include "Animation.h"
#include "Camera.h"
#include "CashInteraction.h"
#include "Payment.h"
#include "HeldDrink.h"
#include "SceneOptions.h"
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

    floorLocation_ = glGetUniformLocation(program_, "uFloor");
    reflectionPassLocation_ = glGetUniformLocation(program_, "uReflectionPass");
    wireframeLocation_ = glGetUniformLocation(program_, "uWireframe");
    textureEnabledLocation_ = glGetUniformLocation(program_, "uUseTexture");
    cameraLocation_ = glGetUniformLocation(program_, "uCameraPosition");
    reflectionSamplerLocation_ = glGetUniformLocation(program_, "uReflectionTexture");
    viewportLocation_ = glGetUniformLocation(program_, "uViewportSize");
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
    createProductTextures();
}

Renderer::~Renderer() {
    glDeleteTextures(12, productTextures_);
    glDeleteTextures(1, &reflectionTexture_);
    glDeleteRenderbuffers(1, &reflectionDepth_);
    glDeleteFramebuffers(1, &reflectionFbo_);
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
    if (suppressGeometry_) return;
    glUniform1i(floorLocation_, floorMaterial_);
    glUniform1i(reflectionPassLocation_, reflectionPass_);
    glUniform1i(wireframeLocation_, wireframe_);
    glUniform1i(textureEnabledLocation_, useTexture_);
    if (useTexture_) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, activeTexture_); }
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
    if (reflectionPass_) return;
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
                           const glm::vec3& position, bool selected, float opening) {
    activeTexture_ = productTextures_[std::clamp(product.id - 1, 0, 11)];
    const float brightness = selected ? 1.16f : 1.0f;
    const glm::vec3 color = glm::min(product.color * brightness, glm::vec3(1.0f));
    switch (product.type) {
    case ProductType::Can: {
        useTexture_ = true;
        drawCylinder(viewProjection, position, {0.34f, 0.58f, 0.34f}, color);
        useTexture_ = false;
        drawCylinder(viewProjection, {position.x, position.y + 0.294f, position.z},
                     {0.34f, 0.025f, 0.34f}, {0.66f, 0.70f, 0.70f});
        drawCylinder(viewProjection, {position.x, position.y - 0.294f, position.z},
                     {0.34f, 0.018f, 0.34f}, {0.56f, 0.59f, 0.59f});
        const auto base = parentTransform_;
        // Dark drinking opening and a hinged metal flap stay visible after opening.
        if (opening > 0.42f) {
            drawCylinder(viewProjection, position + glm::vec3(0, 0.312f, 0.065f),
                         {0.075f, 0.005f, 0.105f}, {0.065f, 0.075f, 0.07f});
            parentTransform_ = base * glm::translate(glm::mat4(1), position + glm::vec3(0, 0.305f, 0.015f)) *
                glm::rotate(glm::mat4(1), glm::radians(65.0f * std::min((opening - 0.42f) / 0.25f, 1.0f)), glm::vec3(1, 0, 0));
            drawBox(viewProjection, {0, -0.01f, 0.04f}, {0.055f, 0.008f, 0.07f}, {0.57f, 0.63f, 0.62f});
        }
        parentTransform_ = base * glm::translate(glm::mat4(1), position + glm::vec3(0, 0.315f, -0.04f)) *
            glm::rotate(glm::mat4(1), glm::radians(-70.0f * std::clamp((opening - 0.18f) / 0.45f, 0.0f, 1.0f)), glm::vec3(1, 0, 0));
        for (int segment = 0; segment < 16; ++segment) {
            const float a = segment * glm::two_pi<float>() / 16;
            drawBox(viewProjection, {0.037f * std::cos(a), 0, 0.04f + 0.052f * std::sin(a)},
                    {0.017f, 0.012f, 0.021f}, {0.78f, 0.82f, 0.81f});
        }
        parentTransform_ = base;
        if (opening > 0.42f && opening < 0.95f) {
            for (int bubble = 0; bubble < 6; ++bubble) {
                const float rise = std::fmod(opening * 3 + bubble * 0.13f, 0.45f);
                drawCylinder(viewProjection, position + glm::vec3(std::sin(bubble * 2.0f) * 0.06f, 0.34f + rise, 0.06f),
                             {0.018f, 0.025f, 0.018f}, {0.85f, 0.96f, 0.94f});
            }
        }
        break;
    }
    case ProductType::Bottle: {
        useTexture_ = true;
        drawCylinder(viewProjection, {position.x, position.y - 0.045f, position.z},
                     {0.30f, 0.43f, 0.30f}, color);
        useTexture_ = false;
        drawCylinder(viewProjection, {position.x, position.y + 0.245f, position.z},
                     {0.30f, 0.18f, 0.30f}, color, true);
        drawCylinder(viewProjection, {position.x, position.y + 0.36f, position.z},
                     {0.13f, 0.12f, 0.13f}, color);
        if (opening < 1.0f) {
            const auto base = parentTransform_;
            const float twist = std::clamp((opening - 0.15f) / 0.65f, 0.0f, 1.0f);
            const float remove = std::clamp((opening - 0.8f) / 0.2f, 0.0f, 1.0f);
            parentTransform_ = base * glm::translate(glm::mat4(1), position + glm::vec3(remove * 0.30f, 0.43f + twist * 0.16f + remove * 0.15f, 0)) *
                glm::rotate(glm::mat4(1), twist * glm::two_pi<float>() * 4, glm::vec3(0, 1, 0));
            const glm::vec3 capColor = product.id == 6 ? glm::vec3(0.90f, 0.25f, 0.05f) : glm::vec3(0.12f, 0.32f, 0.72f);
            drawCylinder(viewProjection, {0, 0, 0}, {0.15f, 0.055f, 0.15f}, capColor);
            drawBox(viewProjection, {0, 0.031f, 0}, {0.11f, 0.005f, 0.018f}, {0.8f, 0.85f, 0.9f});
            parentTransform_ = base;
        } else {
            drawCylinder(viewProjection, position + glm::vec3(0, 0.425f, 0), {0.10f, 0.006f, 0.10f}, {0.13f, 0.22f, 0.23f});
        }

        break;
    }
    case ProductType::Cup:
        if (opening < 1.0f) {
        drawCylinder(viewProjection, {position.x, position.y - 0.025f, position.z},
                     {0.40f, 0.50f, 0.40f}, color, true);
        drawCylinder(viewProjection, {position.x, position.y + 0.235f, position.z},
                     {0.44f, 0.045f, 0.44f}, {0.96f, 0.94f, 0.86f});
        drawCylinder(viewProjection, {position.x, position.y + 0.27f, position.z},
                     {0.38f, 0.035f, 0.38f}, color);
        }
        drawCylinder(viewProjection, {position.x, position.y - 0.025f, position.z},
                     {0.40f, 0.50f, 0.40f}, color, true);
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
                      const Camera& camera, const CashInteraction& cash, const Payment& payment, const HeldDrink& drink, const SceneOptions& options, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (!reflectionPass_ && !options.wireframe) {
        GLint previousFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
        ensureReflectionTarget(std::max(width / 2, 1), std::max(height / 2, 1));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, productTextures_[0]);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, reflectionFbo_);
        reflectionPass_ = true;
        render(machine, animation, camera, cash, payment, drink, options, reflectionWidth_, reflectionHeight_);
        reflectionPass_ = false;
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFbo));
    }
    wireframe_ = options.wireframe;
    glPolygonMode(GL_FRONT_AND_BACK, wireframe_ ? GL_LINE : GL_FILL);
    const bool cutaway = options.xray || (options.cinematic &&
        (animation.active() || (camera.presentationActive() && !camera.external())));
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.97f, 0.95f, 0.90f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 reflection(1.0f);
    if (reflectionPass_) { reflection[1][1] = -1; reflection[3][1] = -0.2f; }
    const glm::mat4 viewProjection =
        camera.projectionMatrix(static_cast<float>(width) / static_cast<float>(height)) *
        camera.viewMatrix() * reflection;
    glUseProgram(program_);
    glUniform1i(glGetUniformLocation(program_, "uLabelTexture"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, productTextures_[0]);
    glUniform1i(reflectionSamplerLocation_, 1);
    glUniform2f(viewportLocation_, static_cast<float>(width), static_cast<float>(height));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, reflectionPass_ || !reflectionTexture_ ? productTextures_[0] : reflectionTexture_);
    glActiveTexture(GL_TEXTURE0);
    glUniform3f(lightLocation_, -9.0f, 14.0f, 8.0f);
    const auto eye = camera.position();
    glUniform3fv(cameraLocation_, 1, &eye[0]);
    glBindVertexArray(vao_);

    const glm::vec3 mint{0.49f, 0.78f, 0.68f};
    const glm::vec3 mintLight{0.65f, 0.86f, 0.77f};
    const glm::vec3 cream{0.96f, 0.93f, 0.84f};
    const glm::vec3 deepMint{0.20f, 0.43f, 0.36f};

    drawEnvironment(viewProjection);
    // Layered contact shadows keep the cabinet grounded without a hard dark disk.
    if (!reflectionPass_) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (int layer = 0; layer < 8; ++layer)
            drawMesh(viewProjection, {0.10f, -0.097f + layer * 0.0001f, -0.05f},
                     {5.2f - layer * 0.2f, 0.001f, 2.8f - layer * 0.14f},
                     {0.23f, 0.28f, 0.25f}, cylinderFirst_, cylinderCount_, 0.024f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    suppressGeometry_ = cutaway;
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

    suppressGeometry_ = false;
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
        const bool highlighted = i == machine.selectedIndex() || i == machine.hoveredIndex();
        // A rotating helix and carriage sit behind each product.
        const auto original = parentTransform_;
        const bool pushing = animation.active() && animation.productIndex() == i;
        const float push = pushing ? std::min(animation.progress() * 3.0f, 1.0f) : 0;
        drawBox(viewProjection, {position.x, position.y - 0.12f, -0.25f + push * 0.55f},
                {0.23f, 0.28f, 0.035f}, {0.56f, 0.64f, 0.59f});
        const float phase = animation.active() && animation.productIndex() == i ? animation.progress() * 20.0f : 0;
        for (int segment = 0; segment < 36; ++segment) {
            const float t = segment / 35.0f;
            const float angle = t * glm::two_pi<float>() * 3 + phase;
            drawBox(viewProjection, {position.x + 0.12f * std::cos(angle),
                position.y - 0.23f + 0.07f * std::sin(angle), -0.22f + t * 0.60f},
                {0.035f, 0.035f, 0.045f}, {0.58f, 0.65f, 0.62f});
        }
        if (product.stock > 0) drawProduct(viewProjection, product, position, highlighted);
        if (animation.active() && animation.productIndex() == i) {
            const auto moving = animation.deliveryPosition(position);
            parentTransform_ = glm::translate(glm::mat4(1), moving) *
                glm::rotate(glm::mat4(1), glm::radians(animation.deliveryRotation()), glm::vec3(0, 0, 1));
            drawProduct(viewProjection, product, glm::vec3(0), false);
            parentTransform_ = original;
            drawBox(viewProjection, {position.x, position.y - 0.36f, 0.50f},
                {0.48f, 0.035f, 0.30f}, {0.35f, 0.95f, 0.70f}, 0.8f);
        }
    }

    const std::array<const char*, 12> labels{{
        "COLA", "SODA", "ENERGY", "LEMON", "WATER", "ORANGE",
        "APPLE", "PEACH", "MILK TEA", "MILK", "COFFEE", "MATCHA"}};
    for (int i = 0; i < 12; ++i) {
        const auto position = machine.productPosition(i);
        const auto& product = machine.products()[static_cast<std::size_t>(i)];
        char tag[24];
        std::snprintf(tag, sizeof(tag), "%s %.0fK", productCode(i).c_str(), product.price / 1000.0f);
        drawBox(viewProjection, {position.x, position.y - 0.49f, 0.825f},
                {0.73f, 0.13f, 0.025f}, cream);
        drawText(viewProjection, tag, {position.x, position.y - 0.49f, 0.844f}, 0.075f, deepMint);
        if (product.stock > 0)
            drawText(viewProjection, labels[i], {position.x, position.y, 0.652f}, 0.065f, deepMint);
    }

    // Angled internal chute and an optional dotted drop path for structural viewing.
    parentTransform_ = glm::translate(glm::mat4(1), {-0.43f, 0.86f, 0.25f}) *
        glm::rotate(glm::mat4(1), glm::radians(-7.0f), glm::vec3(0, 0, 1));
    drawBox(viewProjection, glm::vec3(0), {3.0f, 0.055f, 0.55f}, {0.62f, 0.71f, 0.67f});
    parentTransform_ = glm::mat4(1);
    if (cutaway) {
        const auto selected = machine.productPosition(machine.selectedIndex());
        for (int i = 0; i < 18; ++i) {
            const float t = i / 17.0f;
            drawCylinder(viewProjection, {selected.x, glm::mix(selected.y, 0.9f, t), 1.0f},
                         {0.022f, 0.05f, 0.022f}, {0.2f, 0.82f, 0.63f});
        }
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

    drawText(viewProjection, labels[machine.selectedIndex()], {1.69f, 4.66f, 0.82f}, 0.060f, deepMint);

    if (const Product* selected = machine.selectedProduct()) {
        char price[24];
        std::snprintf(price, sizeof(price), "%s %.0fK", productCode(machine.selectedIndex()).c_str(), selected->price / 1000.0f);
        drawText(viewProjection, price, {1.69f, 4.54f, 0.82f}, 0.065f, deepMint);
    }

    const Product* selectedItem = machine.selectedProduct();
    const char* lcdState = animation.active() ? "VENDING" : animation.delivered() ? "THANK YOU" :
        selectedItem && selectedItem->stock <= 0 ? "SOLD OUT" :
        selectedItem && payment.balance() < selectedItem->price ? "ADD CASH" : "CONFIRM";
    drawText(viewProjection, lcdState, {1.69f, 4.80f, 0.82f}, 0.055f, deepMint);
    char credit[24];
    if (cash.isChange() || cash.state() == CashInteraction::State::Returning ||
        cash.state() == CashInteraction::State::AwaitingCollection || cash.state() == CashInteraction::State::Collecting)
        std::snprintf(credit, sizeof(credit), "RETURN %d", cash.amount());
    else std::snprintf(credit, sizeof(credit), "%.0f VND", payment.balance());
    drawText(viewProjection, credit, {1.69f, 4.40f, 0.82f}, 0.060f, deepMint);

    char stock[20];
    std::snprintf(stock, sizeof(stock), "STOCK %d", selectedItem ? selectedItem->stock : 0);
    drawText(viewProjection, stock, {1.69f, 4.29f, 0.82f}, 0.05f, deepMint);

    for (int key = 0; key < 12; ++key) {
        const int row = key / 3;
        const int column = key % 3;
        const float x = 1.46f + static_cast<float>(column) * 0.22f;
        const float y = 3.70f - static_cast<float>(row) * 0.22f;
        const float depth = animation.buttonDepth(key);
        const bool lit = key == machine.selectedIndex();
        const bool soldOut = machine.products()[key].stock <= 0;
        drawBox(viewProjection, {x, y, 0.72f - depth}, {0.15f, 0.15f, 0.08f},
                {0.77f, 0.87f, 0.79f});
        drawBox(viewProjection, {x, y, 0.765f - depth}, {0.14f, 0.12f, 0.012f},
                soldOut ? glm::vec3(0.67f, 0.66f, 0.61f) : lit ? glm::vec3(0.56f, 0.94f, 0.69f) : cream,
                lit && !soldOut ? 0.3f : 0.0f);
    }

    for (int i = 0; i < 12; ++i) {
        char number[4];
        std::snprintf(number, sizeof(number), "%s", productCode(i).c_str());
        drawText(viewProjection, number,
                 {1.46f + (i % 3) * 0.22f, 3.70f - (i / 3) * 0.22f, 0.78f - animation.buttonDepth(i)},
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

    if (!cutaway)
        drawBox(viewProjection, {-0.43f, 0.62f, 0.85f}, {3.18f, 0.98f, 0.12f}, mint);

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

    suppressGeometry_ = cutaway;
    // A soft mint pane over the products suggests clear reflective glass.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    drawMesh(viewProjection, {-0.43f, 2.78f, 1.25f}, {3.10f, 4.58f, 0.018f},
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

    suppressGeometry_ = false;
    if (animation.delivered()) {
        const auto moving = animation.deliveryPosition(machine.productPosition(animation.productIndex()));
        parentTransform_ = glm::translate(glm::mat4(1), moving) *
            glm::rotate(glm::mat4(1), glm::radians(animation.deliveryRotation()), glm::vec3(0, 0, 1));
        drawProduct(viewProjection, machine.products()[animation.productIndex()], glm::vec3(0), false);
        parentTransform_ = glm::mat4(1);
        drawBox(viewProjection, {-0.43f, 0.83f, 1.02f}, {0.65f, 0.035f, 0.03f}, {0.32f, 0.95f, 0.56f}, 0.8f);
    }
    // Side table and recycling bin are reachable in the world.
    drawBox(viewProjection, {4.8f, 2.1f, 0.8f}, {2.6f, 0.16f, 1.7f}, cream);
    for (float x : {3.7f, 5.9f}) for (float z : {0.15f, 1.45f})
        drawBox(viewProjection, {x, 1.0f, z}, {0.12f, 2.1f, 0.12f}, deepMint);
    // Rounded recycling station: inset mint facade, raised hood and recessed opening.
    drawBox(viewProjection, {-4.9f, 0.04f, 0.4f}, {1.22f, 0.15f, 1.12f}, deepMint);
    drawBox(viewProjection, {-4.9f, 0.84f, 0.4f}, {1.15f, 1.55f, 1.04f}, cream);
    drawBox(viewProjection, {-4.9f, 0.85f, 0.934f}, {0.98f, 1.27f, 0.035f}, mint);
    drawBox(viewProjection, {-4.9f, 1.65f, 0.4f}, {1.20f, 0.12f, 1.1f}, deepMint);
    drawBox(viewProjection, {-4.9f, 1.718f, 0.4f}, {0.84f, 0.02f, 0.72f}, {0.075f, 0.13f, 0.11f});
    drawBox(viewProjection, {-5.43f, 1.84f, 0.4f}, {0.14f, 0.34f, 1.05f}, cream);
    drawBox(viewProjection, {-4.37f, 1.84f, 0.4f}, {0.14f, 0.34f, 1.05f}, cream);
    drawBox(viewProjection, {-4.9f, 1.84f, -0.07f}, {1.08f, 0.34f, 0.12f}, mintLight);
    drawBox(viewProjection, {-4.9f, 2.04f, 0.4f}, {1.24f, 0.14f, 1.12f}, cream);
    drawBox(viewProjection, {-4.9f, 1.988f, 0.4f}, {0.83f, 0.015f, 0.50f}, {0.73f, 0.92f, 0.81f}, 0.25f);
    drawText(viewProjection, "RECYCLE", {-4.9f, 0.65f, 0.966f}, 0.11f, deepMint);
    drawText(viewProjection, "CANS - BOTTLES", {-4.9f, 0.40f, 0.966f}, 0.055f, deepMint);
    for (int side = 0; side < 3; ++side) {
        parentTransform_ = glm::translate(glm::mat4(1), {-4.9f, 1.05f, 0.97f}) *
            glm::rotate(glm::mat4(1), glm::radians(side * 120.0f), glm::vec3(0, 0, 1));
        drawBox(viewProjection, {0, 0.14f, 0}, {0.24f, 0.045f, 0.012f}, deepMint);
        drawBox(viewProjection, {0.11f, 0.11f, 0}, {0.045f, 0.11f, 0.014f}, deepMint);
    }
    parentTransform_ = glm::mat4(1);
    if (drink.placed()) {
        const auto frame = glm::inverse(camera.viewMatrix());
        const glm::vec3 start(frame * glm::vec4(0.34f, -0.34f, -1.05f, 1));
        const float t = drink.placementProgress();
        parentTransform_ = glm::translate(glm::mat4(1), glm::mix(start, drink.placedPosition(), t * t * (3 - 2 * t)));
        drawProduct(viewProjection, drink.product(), glm::vec3(0), false, drink.openingProgress());
        parentTransform_ = glm::mat4(1);
    }
    if (drink.tossing()) {
        const auto frame = glm::inverse(camera.viewMatrix());
        const glm::vec3 start(frame * glm::vec4(0.34f, -0.34f, -1.05f, 1));
        const float t = glm::clamp(drink.tossProgress(), 0.0f, 1.0f);
        const auto position = glm::mix(start, drink.trashTarget(), t) + glm::vec3(0, std::sin(t * glm::pi<float>()) * 0.7f, 0);
        parentTransform_ = glm::translate(glm::mat4(1), position) * glm::rotate(glm::mat4(1), t * 4, glm::vec3(1, 0, 1));
        drawProduct(viewProjection, drink.product(), glm::vec3(0), false, 1);
        parentTransform_ = glm::mat4(1);
    }
    if (!reflectionPass_ && !camera.presentationActive() && !cash.active() && !animation.pressing()) drawHeldDrink(viewProjection, camera, drink);
    if (!reflectionPass_) {
        drawPressHand(viewProjection, camera, animation);
        drawCash(viewProjection, camera, cash);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
    const bool returning = cash.state() == CashInteraction::State::Returning ||
        cash.state() == CashInteraction::State::AwaitingCollection || cash.state() == CashInteraction::State::Collecting;
    const float p = cash.progress();
    const auto ease = [](float t) { t = glm::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); };
    float reach = 0.0f;
    glm::vec3 slot{1.69f, returning ? 1.91f : 2.55f, 0.98f};
    if (cash.state() == CashInteraction::State::Inserting)
        reach = ease(p / 0.45f) * (1.0f - ease((p - 0.82f) / 0.18f));
    if (returning) reach = cash.state() == CashInteraction::State::Collecting ? 1.0f - ease(p) : 1.0f;
    const auto handPosition = heldPosition;
    parentTransform_ = glm::translate(glm::mat4(1), handPosition) * glm::mat4_cast(heldRotation);
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
    // Only the banknotes travel; the hand remains beside the camera.
    glm::vec3 billPosition = glm::mix(heldPosition, slot, reach);
    if (cash.state() == CashInteraction::State::Inserting && p > 0.45f)
        billPosition = slot + glm::vec3(0, 0, -0.55f * ease((p - 0.45f) / 0.43f));
    if (cash.state() == CashInteraction::State::Returning)
        billPosition = slot + glm::vec3(0, 0, -0.40f * (1.0f - ease(p)));
    if (cash.state() != CashInteraction::State::Inserting || p < 0.90f) {
        const auto billRotation = cash.state() == CashInteraction::State::Inserting && p > 0.45f ? slotRotation : glm::slerp(heldRotation, slotRotation, reach);
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

void Renderer::drawPressHand(const glm::mat4& vp, const Camera& camera, const Animation& animation) {
    if (!animation.pressing()) return;
    const int key = animation.pressedIndex();
    const float p = animation.pressProgress();
    const auto smooth = [](float t) { t = glm::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); };
    const float reach = smooth(p / 0.44f) * (1 - smooth((p - 0.69f) / 0.31f));
    const auto cameraFrame = glm::inverse(camera.viewMatrix());
    const auto rest = cameraFrame * glm::vec4(0.38f, -0.50f, -1.0f, 1);
    const glm::vec3 button{1.46f + key % 3 * 0.22f, 3.70f - key / 3 * 0.22f,
                           0.80f - animation.buttonDepth(key)};
    const auto palm = glm::mix(glm::vec3(rest), button + glm::vec3(0, -0.19f, 0.10f), reach);
    parentTransform_ = glm::translate(glm::mat4(1), palm);
    const glm::vec3 skin{0.83f, 0.61f, 0.44f};
    drawBox(vp, {0, -0.20f, 0.10f}, {0.18f, 0.30f, 0.16f}, {0.27f, 0.49f, 0.42f});
    drawBox(vp, {0, 0, 0.045f}, {0.22f, 0.20f, 0.11f}, skin);
    // Extended index fingertip touches the key; other fingers curl into the palm.
    drawBox(vp, {0, 0.13f, -0.015f}, {0.045f, 0.17f, 0.05f}, skin);
    drawBox(vp, {0, 0.19f, -0.075f}, {0.047f, 0.055f, 0.10f}, skin);
    for (int i = 0; i < 3; ++i)
        drawBox(vp, {0.05f + i * 0.045f, 0.06f, 0.07f}, {0.04f, 0.10f, 0.08f}, skin);
    drawBox(vp, {-0.13f, -0.01f, 0.03f}, {0.09f, 0.06f, 0.06f}, skin);
    parentTransform_ = glm::mat4(1);
}

void Renderer::drawHeldDrink(const glm::mat4& vp, const Camera& camera, const HeldDrink& drink) {
    if (!drink.holding()) return;
    const auto cameraFrame = glm::inverse(camera.viewMatrix());
    const float p = drink.openingProgress();
    const float examine = drink.opening() ? std::sin(p * glm::pi<float>()) : 0;
    const float sip = drink.drinkBlend();
    auto localPosition = glm::mix(glm::vec3(0.34f, -0.34f, -1.05f), glm::vec3(0.06f, -0.14f, -0.60f), sip);
    localPosition = glm::mix(localPosition, glm::vec3(0.10f, -0.20f, -0.86f), examine);
    localPosition = glm::mix(localPosition, glm::vec3(0, -0.08f, -drink.inspectDistance()), drink.inspectBlend());
    glm::vec3 worldPosition(cameraFrame * glm::vec4(localPosition, 1));
    auto heldRotation = glm::quat_cast(glm::mat3(cameraFrame)) *
        glm::angleAxis(glm::radians(15.0f + 60.0f * sip + examine * 15.0f), glm::vec3(1, 0, 0));
    const auto gripRotation = heldRotation;
    heldRotation *= glm::angleAxis(glm::radians(drink.inspectYaw() * drink.inspectBlend()), glm::vec3(0, 1, 0)) *
                    glm::angleAxis(glm::radians(drink.inspectPitch() * drink.inspectBlend()), glm::vec3(1, 0, 0));
    if (drink.pickingUp()) {
        const float t = drink.pickupProgress();
        const float smooth = t * t * (3 - 2 * t);
        worldPosition = glm::mix(drink.trayPosition(), worldPosition, smooth);
        heldRotation = glm::slerp(drink.pickupFromTable() ? glm::quat(1, 0, 0, 0) : glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 0, 1)), heldRotation, smooth);
    }
    parentTransform_ = glm::translate(glm::mat4(1), worldPosition) * glm::mat4_cast(heldRotation);
    const auto bottleFrame = parentTransform_;
    drawProduct(vp, drink.product(), glm::vec3(0), false, p);
    if (drink.time() < 3.5f) {
        for (int i = 0; i < 8; ++i) {
            const float rise = std::fmod(drink.time() * 0.15f + i * 0.07f, 0.45f);
            drawCylinder(vp, {std::sin(i * 2.4f) * 0.18f, 0.3f + rise, std::cos(i * 2.4f) * 0.12f},
                         {0.014f, 0.045f, 0.014f}, {0.87f, 0.96f, 0.97f});
        }
    }
    parentTransform_ = glm::translate(glm::mat4(1), worldPosition) *
        glm::mat4_cast(drink.pickingUp() ? heldRotation : gripRotation);
    const glm::vec3 skin{0.83f, 0.61f, 0.44f};
    // Supporting hand stays wrapped around the body throughout opening and drinking.
    drawBox(vp, {0.14f, -0.16f, 0.13f}, {0.15f, 0.20f, 0.10f}, skin);
    drawBox(vp, {0.22f, -0.34f, 0.18f}, {0.17f, 0.30f, 0.16f}, {0.27f, 0.49f, 0.42f});
    for (int i = 0; i < 4; ++i)
        drawBox(vp, {0.06f, -0.20f + i * 0.05f, 0.17f}, {0.18f, 0.04f, 0.055f}, skin);
    drawBox(vp, {0.15f, -0.035f, -0.04f}, {0.065f, 0.14f, 0.06f}, skin);
    if (drink.opening()) {
        const bool bottle = drink.product().type == ProductType::Bottle;
        const float reach = std::clamp(p / 0.20f, 0.0f, 1.0f) * (1 - std::clamp((p - 0.90f) / 0.10f, 0.0f, 1.0f));
        const float twist = std::clamp((p - 0.15f) / 0.65f, 0.0f, 1.0f);
        const float remove = std::clamp((p - 0.8f) / 0.2f, 0.0f, 1.0f);
        const glm::vec3 target = bottle ? glm::vec3(remove * 0.30f, 0.49f + twist * 0.16f + remove * 0.15f, 0) : glm::vec3(0, 0.39f, 0.07f);
        const auto handPosition = glm::mix(glm::vec3(-0.45f, -0.20f, 0.05f), target, reach);
        parentTransform_ = bottleFrame * glm::translate(glm::mat4(1), handPosition);
        if (bottle) parentTransform_ *= glm::rotate(glm::mat4(1), twist * glm::two_pi<float>() * 4, glm::vec3(0, 1, 0));
        drawBox(vp, {-0.10f, 0.06f, 0}, {0.18f, 0.10f, 0.14f}, skin);
        drawBox(vp, {-0.26f, 0.07f, 0}, {0.24f, 0.13f, 0.14f}, {0.27f, 0.49f, 0.42f});
        for (int finger = 0; finger < 3; ++finger)
            drawBox(vp, {0.01f, 0.02f, -0.05f + finger * 0.05f}, {0.13f, 0.035f, 0.04f}, skin);
        drawBox(vp, {0.02f, -0.04f, 0.07f}, {0.11f, 0.04f, 0.05f}, skin);
    }
    parentTransform_ = glm::mat4(1);
}

void Renderer::createProductTextures() {
    constexpr int width = 512, height = 256;
    const auto products = createProducts();
    const std::array<const char*, 12> names{{"COLA", "SODA", "ENERGY", "LEMON", "WATER", "ORANGE", "APPLE", "PEACH", "MILK TEA", "MILK", "COFFEE", "MATCHA"}};
    glGenTextures(12, productTextures_);
    for (int index = 0; index < 12; ++index) {
        std::vector<unsigned char> pixels(width * height * 3);
        auto paint = [&](int x, int y, glm::vec3 color) {
            x = (x % width + width) % width;
            if (y < 0 || y >= height) return;
            for (int channel = 0; channel < 3; ++channel)
                pixels[(y * width + x) * 3 + channel] = static_cast<unsigned char>(glm::clamp(color[channel], 0.0f, 1.0f) * 255);
        };
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
            auto color = products[index].color;
            if ((x + y / 2) % 90 < 12) color = glm::mix(color, glm::vec3(1), 0.28f);
            if (y > 88 && y < 164) color = {0.97f, 0.95f, 0.86f};
            paint(x, y, color);
        }
        auto lettering = [&](const std::string& text, int center, int y, int scale) {
            const int left = center - static_cast<int>(text.size()) * 6 * scale / 2;
            for (int i = 0; i < static_cast<int>(text.size()); ++i) {
                const auto code = alphabet.find(text[i]);
                if (code == std::string::npos) continue;
                for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col)
                    if (glyphs[code][row] & (1 << (4 - col)))
                        for (int dx = 0; dx < scale; ++dx) for (int dy = 0; dy < scale; ++dy)
                            paint(left + (i * 6 + col) * scale + dx, y + (6 - row) * scale + dy, {0.13f, 0.28f, 0.23f});
            }
        };
        for (int center : {0, 128, 256, 384}) {
            lettering(names[index], center, 130, 2);
            lettering(productCode(index) + "  " + std::to_string(static_cast<int>(products[index].price / 1000)) + "K", center, 105, 1);
        }
        glBindTexture(GL_TEXTURE_2D, productTextures_[index]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::ensureReflectionTarget(int width, int height) {
    if (reflectionFbo_ && width == reflectionWidth_ && height == reflectionHeight_) return;
    if (!reflectionFbo_) {
        glGenFramebuffers(1, &reflectionFbo_);
        glGenTextures(1, &reflectionTexture_);
        glGenRenderbuffers(1, &reflectionDepth_);
    }
    reflectionWidth_ = width; reflectionHeight_ = height;
    glBindTexture(GL_TEXTURE_2D, reflectionTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionDepth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTexture_, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflectionDepth_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Could not create floor reflection framebuffer");
}

void Renderer::drawEnvironment(const glm::mat4& vp) {
    const glm::vec3 plaster{0.91f, 0.92f, 0.89f};
    const glm::vec3 stone{0.78f, 0.81f, 0.78f};
    const glm::vec3 metal{0.24f, 0.31f, 0.34f};
    const glm::vec3 oak{0.63f, 0.44f, 0.27f};
    if (!reflectionPass_) {
        floorMaterial_ = true;
        drawMesh(vp, {0, -0.16f, 10}, {38, 0.12f, 32}, {0.86f, 0.87f, 0.82f}, 0, 36);
        floorMaterial_ = false;
    }
    // A bright university lobby: VKU blue/yellow accents, louvers and a glazed garden edge.
    drawBox(vp, {0, 5.9f, -3.2f}, {8.2f, 12, 0.22f}, plaster);
    drawBox(vp, {0, 0.16f, -3.04f}, {8.2f, 0.5f, 0.12f}, stone);
    for (float x : {-18.8f, 18.8f}) drawBox(vp, {x, 5.8f, 10}, {0.24f, 12, 32}, plaster);
    drawBox(vp, {0, 11.8f, 9.5f}, {38, 0.22f, 33}, {0.94f, 0.94f, 0.89f});
    drawBox(vp, {0, 6.8f, -3.04f}, {6.4f, 0.045f, 0.10f}, {0.78f, 0.62f, 0.30f});
    drawText(vp, "V", {-0.58f, 8.0f, -2.97f}, 0.92f, {0.88f, 0.28f, 0.19f});
    drawText(vp, "K", {0, 8.0f, -2.97f}, 0.92f, {0.96f, 0.67f, 0.15f});
    drawText(vp, "U", {0.58f, 8.0f, -2.97f}, 0.92f, {0.14f, 0.43f, 0.68f});
    drawText(vp, "CAMPUS LOUNGE", {0, 7.05f, -2.97f}, 0.20f, metal);
    drawBox(vp, {0, 10.7f, -3.0f}, {37.4f, 0.20f, 0.24f}, {0.92f, 0.70f, 0.24f});
    drawBox(vp, {0, 10.16f, -3.02f}, {37.4f, 0.80f, 0.18f}, {0.21f, 0.47f, 0.66f});
    drawText(vp, "VIETNAM - KOREA UNIVERSITY", {0, 10.14f, -2.90f}, 0.26f, {0.96f, 0.96f, 0.89f});
    // Garden scenery seen through the large panes, kept well behind the active play area.
    drawBox(vp, {0, 6.3f, -7.2f}, {38, 12, 0.08f}, {0.68f, 0.80f, 0.88f}, 0.18f);
    drawBox(vp, {0, 0.4f, -5.8f}, {38, 0.5f, 3}, {0.56f, 0.67f, 0.40f});
    for (float x : {-17.0f, -12.0f, -7.0f, 7.0f, 12.0f, 17.0f}) {
        drawBox(vp, {x, 4.7f, -3.05f}, {0.14f, 9.4f, 0.20f}, metal);
        drawBox(vp, {x, 9.5f, -3.05f}, {0.30f, 0.20f, 0.30f}, stone);
    }
    for (float x : {-10.7f, 10.7f}) {
        for (float y : {0.18f, 3.2f, 9.4f}) drawBox(vp, {x, y, -3.05f}, {13.3f, 0.12f, 0.20f}, metal);
        // Lower push handles identify full-height glass doors.
        for (float dx : {-0.28f, 0.28f}) drawBox(vp, {x + dx, 2.5f, -2.86f}, {0.055f, 0.80f, 0.08f}, metal);
        for (int slat = 0; slat < 6; ++slat)
            drawBox(vp, {x, 8.25f + slat * 0.18f, -2.85f}, {13.1f, 0.045f, 0.16f}, stone);
    }
    // Ceiling beams and warm recessed strip lights establish depth without clutter.
    for (float z : {1.5f, 8.0f, 15.0f, 22.0f}) {
        drawBox(vp, {0, 11.55f, z}, {37, 0.14f, 0.24f}, stone);
        drawBox(vp, {0, 11.45f, z}, {14, 0.025f, 0.10f}, {1.0f, 0.95f, 0.79f}, 0.45f);
    }
    const auto initial = parentTransform_;
    auto plant = [&](glm::vec3 base, float scale, bool palm) {
        parentTransform_ = glm::translate(glm::mat4(1), base) * glm::scale(glm::mat4(1), glm::vec3(scale));
        const auto frame = parentTransform_;
        drawCylinder(vp, {0, 0.50f, 0}, {1.1f, 1.1f, 1.1f}, {0.82f, 0.80f, 0.71f});
        drawCylinder(vp, {0, 1.035f, 0}, {1.12f, 0.05f, 1.12f}, {0.66f, 0.64f, 0.52f});
        drawCylinder(vp, {0, 1.07f, 0}, {0.99f, 0.015f, 0.99f}, {0.22f, 0.19f, 0.14f});
        drawCylinder(vp, {0, palm ? 2.25f : 1.55f, 0}, {0.12f, palm ? 3.0f : 1.3f, 0.12f}, oak);
        for (int leaf = 0; leaf < (palm ? 10 : 9); ++leaf) {
            const float a = leaf * 2.39996f;
            const float height = palm ? 3.6f : 1.7f + (leaf % 3) * 0.25f;
            const auto crown = frame * glm::translate(glm::mat4(1), {0, height, 0}) *
                glm::rotate(glm::mat4(1), a, glm::vec3(0, 1, 0));
            for (int segment = 0; segment < 5; ++segment) {
                const float t0 = segment / 5.0f, t1 = (segment + 1) / 5.0f;
                auto point = [&](float t) {
                    return glm::vec3((palm ? 1.65f : 0.90f) * t,
                        0.48f * std::sin(t * glm::pi<float>()) - (palm ? 0.60f : 0.25f) * t * t, 0);
                };
                const auto start = point(t0), end = point(t1);
                const auto direction = end - start;
                const float mid = (t0 + t1) * 0.5f;
                parentTransform_ = crown * glm::translate(glm::mat4(1), (start + end) * 0.5f) *
                    glm::rotate(glm::mat4(1), std::atan2(-direction.x, direction.y), glm::vec3(0, 0, 1));
                const float leafWidth = 0.04f + 0.22f * std::sin(mid * glm::pi<float>());
                drawCylinder(vp, glm::vec3(0), {leafWidth, glm::length(direction) * 1.08f, 0.035f},
                    leaf % 2 ? glm::vec3(0.20f, 0.39f, 0.23f) : glm::vec3(0.31f, 0.49f, 0.28f), true);
            }
        }
        parentTransform_ = initial;
    };
    for (float x : {-16.5f, -8.0f, 8.0f, 16.5f}) plant({x, -0.09f, -1.5f}, 1.0f, true);
    for (float x : {-16.2f, 16.2f}) plant({x, -0.09f, 13}, 1.1f, false);
    // Slatted seating and slim legs sit in the distant side zones.
    for (float x : {-12.0f, 12.0f}) {
        for (int slat = 0; slat < 5; ++slat) {
            drawBox(vp, {x, 1.08f, -1.2f + slat * 0.18f}, {4.7f, 0.12f, 0.13f}, oak);
            drawBox(vp, {x, 1.48f + slat * 0.18f, -1.35f}, {4.7f, 0.12f, 0.14f}, oak);
        }
        for (float dx : {-1.7f, 1.7f}) drawBox(vp, {x + dx, 0.48f, -0.85f}, {0.13f, 1.1f, 0.7f}, metal);
        drawBox(vp, {x, -0.06f, -0.85f}, {5.1f, 0.025f, 1.5f}, {0.77f, 0.78f, 0.72f});
    }
    // Transparent glazing, drawn after the opaque exterior scenery.
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    for (float x : {-10.7f, 10.7f})
        drawMesh(vp, {x, 4.7f, -3.0f}, {13.2f, 9.2f, 0.025f}, {0.66f, 0.85f, 0.88f}, 0, 36, 0.11f);
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}
